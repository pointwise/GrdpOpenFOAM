/****************************************************************************
 *
 * (C) 2021 Cadence Design Systems, Inc. All rights reserved worldwide.
 *
 * This sample source code is not supported by Cadence Design Systems, Inc.
 * It is provided freely for demonstration purposes only.
 * SEE THE WARRANTY DISCLAIMER AT THE BOTTOM OF THIS FILE.
 *
 ***************************************************************************/
/****************************************************************************
*
* OpenFOAM Grid Import Plugin (GRDP)
*
***************************************************************************/

#ifndef FOAMFILE_H
#define FOAMFILE_H

#include "apiGRDPUtils.h"
#include "PwpFile.h"

#include <algorithm>
#include <cctype>
#include <functional> 
#include <map>
#include <string>
#include <vector>


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

/*! A base class for reading and parsing all OpenFOAM grid data files.
*/
class FoamFile : public PwpFile {
    enum {
        DefReserve = 128    //!< The default size reserved for token strings
    };

    typedef std::map<std::string, std::string>  StringStringMap;

protected:

    FoamFile(const char *baseName) :
        hdrVals_(),
        baseName_(baseName),
        dataPos_()
    {
    }


public:

    virtual ~FoamFile()
    {
    }

    //! Opens the foam file (in cwd) and loads the header data.
    inline bool     open() {
                        // IMPORTANT! MUST use pwpBinary when opening file to
                        // prevent platform EOL differences from breaking file
                        // position handling.
                        // Also, prior to passing control off to this plugin,
                        // the SDK sets the cwd to the import folder location.
                        // So, we can just open the file without a path!
                        return PwpFile::open(baseName_, pwpRead | pwpBinary) &&
                            readHeader(); }

    //! \return true if header key exists and is equal to expectedVal.
    inline bool     headerValIs(const char *key, const char *expectedVal) {
                        std::string val;
                        return getHeaderVal(key, val) && (expectedVal == val); }


    //! Reads from the file and discards all leading whitespace and comments.
    //! The file pos is left at the first non white space or comment char.
    //! \return false if EOF is encountered.
    bool
    wspaceCommentsSkip()
    {
        // expecting:
        // [whitespace]// some comment text\n
        int c1;
        int c2;
        while (wspaceSkip()) {
            // check for first char of the "//" begin comment sequence
            if (!getcNotEOF(c1)) {
                return false;
            }
            if ('/' != c1) {
                // not a comment - restore char and stop processing
                ungetc(c1);
                break;
            }
            // C standard says ungetc() is only guaranteed to work once! To
            // properly restore the file pos if the second char is not a
            // comment, we must capture the rewind pos.
            sysFILEPOS rewPos;
            if (!getPos(rewPos)) {
                return false;
            }
            // check for second char of the "//" or "/*" begin comment sequence
            if (!getcNotEOF(c2)) {
                return false;
            }
            if ('/' == c2) {
                // We found a C++ style comment - discard rest of line.
                if (!skipToChar('\n')) {
                    return false;
                }
                // look for another comment
                continue;
            }
            else if ('*' == c2) {
                // We found a C style comment. Discard all until end of comment.
                // C style comments must be closed. This will run until EOF
                // (an error) or end of comment.
                while ('/' != c2) {
                    if (!skipToChar('*')) {
                        return false;
                    }
                    if (!getcNotEOF(c2)) {
                        return false;
                    }
                    // skip consecutive '*' chars
                    while ('*' == c2) {
                        // get next char
                        if (!getcNotEOF(c2)) {
                            return false;
                        }
                    }
                }
                // look for another comment
                continue;
            }
            // Not a comment, restore file pos to c2's position
            if (!setPos(rewPos)) {
                // could not rewind pos to c2!
                return false;
            }
            // With c2's position restored, we can now put back c1
            ungetc(c1);
            break;
        }
        return true;
    }


    //! Reads and caches the header key/value pairs and leaves the file pos on
    //! the first non white space char after the header.
    //! \return false if header could not be read.
    //! \sa rewindToBeginData(), afterReadHeader()
    bool readHeader()
    {
        // FoamFile
        // {
        //     version     2.0;
        //     format      ascii;
        //     class       faceList;
        //     location    "constant/polyMesh";
        //     object      faces;
        // }
        std::string tok;
        std::string val;
        bool ret = wspaceCommentsSkip() && readAlphaTokenIs("FoamFile") &&
            readTokenIs("{") && wspaceCommentsSkip() && readToken(tok);
        while (ret && ("}" != tok)) {
            // hdrVals_[tok] creates an empty string value in the map. Load
            // trimmed value from the file up to but not including the ;
            if (!readUntilTrim(hdrVals_[tok], ';')) {
                ret = false;
                break;
            }
            ret = wspaceCommentsSkip() && readToken(tok);
        }
        if (ret) {
            // Mark position after header. The call to afterReadHeader() may
            // change this!
            ret = wspaceCommentsSkip() && markBeginData();
        }
        // Give subclass' implementation a chance to process the data loaded
        // from the header.
        return ret && this->afterReadHeader();
    }


    // Gets the value for the given key in val. If key does not exist, defVal is
    //! stored in val.
    //! \return true if val was set. false if key does not exist and defVal is
    //! null (the default).
    bool
    getHeaderVal(const char *key, std::string &val, const char *defVal = 0)
    {
        bool ret = true;
        StringStringMap::const_iterator it = hdrVals_.find(key);
        if (hdrVals_.end() != it) {
            // key exists! Capture mapped value and return true
            val = it->second;
        }
        else if (0 != defVal) {
            // key not found! Silently use default value and return true
            val = defVal;
        }
        else {
            // key not found! No default value provided so return false.
            ret = false;
        }
        return ret;
    }

protected:
    //! Caches the file's current pos. This pos should mark the first valid data
    //! char after the header. This is called by readHeader() prior to calling
    //! afterReadHeader(). It is okay for the subclass implementation of
    //! afterReadHeader() to also call markBeginData().
    //! \return true on success
    //! \sa rewindToBeginData()
    bool    markBeginData() {
                return getPos(dataPos_); }


    //! Rewinds file's current pos to the location marked by the most recent
    //! call to markBeginData().
    //! \return true on success
    //! \sa markBeginData()
    bool    rewindToBeginData() {
                return setPos(dataPos_); }

private:
    //! Implemented by subclasses to validate the file's header information.
    //! This is called by readHeader() after all the file's header has been read
    //! and all key/value pairs have been cached.
    //! \return subclass impl should return true if header is valid.
    //! \sa readHeader(), markBeginData(), rewindToBeginData()
    virtual bool    afterReadHeader() = 0;

private:
    StringStringMap hdrVals_;
    std::string     baseName_;
    sysFILEPOS      dataPos_;
};

#endif  // FOAMFILE_H


/****************************************************************************
 *
 * This file is licensed under the Cadence Public License Version 1.0 (the
 * "License"), a copy of which is found in the included file named "LICENSE",
 * and is distributed "AS IS." TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE
 * LAW, CADENCE DISCLAIMS ALL WARRANTIES AND IN NO EVENT SHALL BE LIABLE TO
 * ANY PARTY FOR ANY DAMAGES ARISING OUT OF OR RELATING TO USE OF THIS FILE.
 * Please see the License for the full text of applicable terms.
 *
 ****************************************************************************/
