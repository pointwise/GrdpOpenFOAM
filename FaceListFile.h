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

#ifndef FACELISTFILE_H
#define FACELISTFILE_H

#include "FoamFile.h"

#include "apiGRDPUtils.h"
#include "apiGridModel.h"
#include "apiPWP.h"


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

/*! A class for reading OpenFOAM faceList files.
*/
class FaceListFile : public FoamFile {
public:

    FaceListFile(const char *baseName) :
        FoamFile(baseName),
        numFaces_(0)
    {
    }

    virtual ~FaceListFile()
    {
    }


    //! \return The number of faces in this file.
    inline PWP_UINT32   getNumFaces() const {
                            return numFaces_; }


    //! Reads the next face from the file into data.
    //! \return true if data contains a valid face. false if face data could not
    //! be read or if an unsupported face type is detected.
    bool readNextFace(PWGM_ASSEMBLER_DATA &data)
    {
        bool ret = readInt(data.vertCnt) && wspaceSkipToChar('(');
        if (ret) {
            switch (data.vertCnt) {
            case 4:
                // each face has form: "4(3 9 10 0)"
                ret = readInt(data.index[0]) && readInt(data.index[1]) &&
                    readInt(data.index[2]) && readInt(data.index[3]) &&
                    wspaceSkipToChar(')');
                break;
            case 3:
                // each face has form: "3(3 9 10)"
                ret = readInt(data.index[0]) && readInt(data.index[1]) &&
                    readInt(data.index[2]) && wspaceSkipToChar(')');
                break;
            default:
                // Unsupported face type!
                // TODO: support other face types
                ret = false;
                break;
            }
        }
        return ret;
    }


private:
    //! Validate header values, capture total face count, leave file pos on
    //! first char after (, and re-mark data begin position.
    virtual bool
    afterReadHeader()
    {
        // HEADER
        // 68        // file pos starts between HEADER and this count
        // (         // file pos ends after this paren
        //  4( 3  9 10  0)
        //  4( 8 24  9  3)
        //    ...snip...
        //  4(44 42 18 17)
        //  4(40 39 42 44)
        // )        
        // EOF
        std::string val;
        return headerValIs("class", "faceList") && readInt(numFaces_) &&
            wspaceSkipToChar('(') && markBeginData();
    }


private:
    PWP_UINT32  numFaces_;  //!< The number of faces in the file
};

#endif // FACELISTFILE_H


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
