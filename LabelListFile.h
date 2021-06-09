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

#ifndef LABELLISTFILE_H
#define LABELLISTFILE_H

#include "FoamFile.h"

#include "apiGRDPUtils.h"
#include "apiPWP.h"


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

/*! A class for reading OpenFOAM labelList files.
*/
class LabelListFile : public FoamFile {
public:

    LabelListFile(const char *baseName) :
        FoamFile(baseName),
        numLbls_(0)
    {
    }

    ~LabelListFile()
    {
    }


    //! Gets the number of label items in the file.
    inline PWP_UINT32   getNumLabels() const {
                            return numLbls_; }


    //! Reads the next label item from the file.
    inline bool         readNextLabel(PWP_UINT32 &lbl) {
                            return readInt(lbl); }

private:
    //! Validate header values, capture total label count, leave file pos on
    //! first char after (, and re-mark data begin position.
    virtual bool
    afterReadHeader()
    {
        // HEADER
        // 68        // file pos starts between HEADER and this count
        // (         // file pos ends after this paren
        //   0  0  1  2  0  1  4  2  4  3
        //   5  6  1  8  8  3  9 10  8  5
        //       ...snip...
        //   6  7  8  9 10 11  8  9 12 13
        //  11 10 15 14 12 13 14 15
        // )
        // EOF
        return headerValIs("class", "labelList") && readInt(numLbls_) &&
            wspaceSkipToChar('(') && markBeginData();
    }


private:
    PWP_UINT32  numLbls_;   //!< The number of label items in the file
};

#endif // LABELLISTFILE_H


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
