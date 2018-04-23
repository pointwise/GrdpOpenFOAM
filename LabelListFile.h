/****************************************************************************
*
* OpenFOAM Grid Import Plugin (GRDP)
*
* Copyright (c) 2012-2018 Pointwise, Inc.
* All rights reserved.
*
* This sample Pointwise plugin is not supported by Pointwise, Inc.
* It is provided freely for demonstration purposes only.
* SEE THE WARRANTY DISCLAIMER AT THE BOTTOM OF THIS FILE.
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
* DISCLAIMER:
* TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, POINTWISE DISCLAIMS
* ALL WARRANTIES, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED
* TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE, WITH REGARD TO THIS SCRIPT. TO THE MAXIMUM EXTENT PERMITTED
* BY APPLICABLE LAW, IN NO EVENT SHALL POINTWISE BE LIABLE TO ANY PARTY
* FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
* WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF
* BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE
* USE OF OR INABILITY TO USE THIS SCRIPT EVEN IF POINTWISE HAS BEEN
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGES AND REGARDLESS OF THE
* FAULT OR NEGLIGENCE OF POINTWISE.
*
***************************************************************************/
