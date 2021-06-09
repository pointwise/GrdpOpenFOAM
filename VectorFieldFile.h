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

#ifndef VECTORFIELDFILE_H
#define VECTORFIELDFILE_H

#include "FoamFile.h"

#include "apiGRDPUtils.h"
#include "apiGridModel.h"

#include <sstream>
#include <vector>


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

/*! A class for reading OpenFOAM vectorField files.
*/
class VectorFieldFile : public FoamFile {

    typedef std::vector<std::string>    StringArray1;

public:

    VectorFieldFile(const char *baseName) :
        FoamFile(baseName),
        numPts_(0)
    {
    }

    virtual ~VectorFieldFile()
    {
    }


    //! Read the vectors from file and store in hVL.
    bool read(GRDP_RTITEM &rti, PWGM_HVERTEXLIST &hVL)
    {
        // afterReadHeader() leaves the file pos on the char AFTER the first (.
        //
        // HEADER
        // 45           // afterReadHeader() reads this value into numPts_; then
        // (            // reads and discards the (; then
        //  (0.5 0 0)   // leaves file pos at start of this line
        //  (0 0 0)
        //  ...snip...
        //  (1.5 1 0.5)
        //  (1.5 0.5 1)
        // )
        // EOF

        bool ret = (0 != numPts_) && PwVlstAllocate(hVL, numPts_);
        if (ret && grdpProgressBeginStep(&rti, numPts_)) {
            PWGM_VERTDATA vert = { 0 };
            // parse all "(v0 v1 v2)" and store in hVL
            std::string xyz;
            for (vert.i = 0; vert.i < numPts_ && ret; ++vert.i) {
                ret = wspaceSkipToChar('(') && readUntil(xyz, ')') &&
                    setVertData(xyz, vert) &&
                    PwVlstSetXYZData(hVL, vert.i, vert) &&
                    grdpProgressIncr(&rti);
            }
            // There should be one ) remaining and then EOF
            ret = ret && wspaceSkipToChar(')') && wspaceCommentsSkip() &&
                wspaceSkipToEOF();
        }
        return grdpProgressEndStep(&rti) && ret;
    }


    inline PWP_UINT32   getNumPts() const {
                            return numPts_; }


private:

    //! Parse the "double double double" string and store in vert
    bool
    setVertData(const std::string &xyz, PWGM_VERTDATA &vert)
    {
        StringArray1 toks;
        bool ret = (3 == tokenize(xyz, toks));
        if (ret) {
            char *endPtr;
            vert.x = std::strtod(toks.at(0).c_str(), &endPtr);
            vert.y = std::strtod(toks.at(1).c_str(), &endPtr);
            vert.z = std::strtod(toks.at(2).c_str(), &endPtr);
        }
        return ret;
    }


    //! Split the space delimited str and store in toks
    static PWP_UINT32
    tokenize(const std::string &str, StringArray1 &toks)
    {
        toks.clear();
        std::string tok;
        std::stringstream ss(str);
        while (ss >> tok) {
            toks.push_back(tok);
        }
        return static_cast<PWP_UINT32>(toks.size());
    }


    //! Validate header values, capture total vector count, leave file pos on
    //! first char after (, and re-mark data begin position.
    virtual bool
    afterReadHeader()
    {
        // HEADER
        // 45        // file pos starts between HEADER and this count
        // (         // file pos ends after this paren
        //  (0.5 0 0)
        //  (0 0 0)
        //  ...snip...
        //  (1.5 1 0.5)
        //  (1.5 0.5 1)
        // )
        // EOF
        return headerValIs("class", "vectorField") && readInt(numPts_) &&
            wspaceSkipToChar('(') && markBeginData();
    }


private:

    PWP_UINT32  numPts_;    //!< The number of vector triples in this file
};

#endif // VECTORFIELDFILE_H


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
