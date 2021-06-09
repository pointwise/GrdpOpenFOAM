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

#include "FaceListFile.h"
#include "LabelListFile.h"
#include "VectorFieldFile.h"

#include "apiGRDP.h"
#include "apiGRDPUtils.h"
#include "apiGridModel.h"
#include "apiPWP.h"
#include "runtimeReadGrid.h"

#include <algorithm> // for swap() < C++11
#include <string>
//#include <utility> // for swap() >= C++11


// Swaps face indices so the face normal is reversed
inline static void
reverseFace(PWGM_ASSEMBLER_DATA &face)
{
    if (4 == face.vertCnt) {
        std::swap(face.index[1], face.index[3]);
    }
    else if (3 == face.vertCnt) {
        std::swap(face.index[1], face.index[2]);
    }
    else {
        // Could not reverse an unsupported face type
        ASSERT(false);
    }
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class OpenFOAMGridReader {
public:

    OpenFOAMGridReader(GRDP_RTITEM &rti) :
        rti_(rti),
        hVL_(PwModCreateUnsVertexList(rti.model)),
        facesFile_("faces"),
        ownerFile_("owner"),
        neighborFile_("neighbour"),
        pointsFile_("points")
    {
    }

    ~OpenFOAMGridReader()
    {
    }


    PWP_BOOL
    read()
    {
        // Open files and do some sanity checks before doing heavy lifting.
        // All faces have owners (numOwners == numFaces).
        // Only internal faces have neighbors (numNeighbors < numFaces)
        const PWP_UINT32 NumMajorSteps = 4;
        return grdpProgressEnd(&rti_, grdpProgressInit(&rti_, NumMajorSteps) &&
            pointsFile_.open() && facesFile_.open() && ownerFile_.open() &&
            neighborFile_.open() &&
            (ownerFile_.getNumLabels() == facesFile_.getNumFaces()) &&
            (neighborFile_.getNumLabels() < facesFile_.getNumFaces()) &&
            pointsFile_.read(rti_, hVL_) && readCells());
    }


private:

    bool readCells()
    {
        const PWP_UINT32 numFaces = facesFile_.getNumFaces();
        PWGM_HBLOCKASSEMBLER hAsm = PwVlstCreateBlockAssembler(hVL_);
        bool ret = PWGM_HBLOCKASSEMBLER_ISVALID(hAsm);
        if (ret && grdpProgressBeginStep(&rti_, numFaces)) {
            PWGM_ASSEMBLER_DATA data;
            PWP_UINT32 ii;
            // The first numNbors faces are interior (have owner and neighbor)
            data.type = PWGM_FACETYPE_INTERIOR;
            const PWP_UINT32 numNbors = neighborFile_.getNumLabels();
            for (ii = 0; ii < numNbors; ++ii) {
                if (!readFaceVertices(data)) {
                    ret = false;
                    break;
                }
                if (!ownerFile_.readNextLabel(data.owner) ||
                        !neighborFile_.readNextLabel(data.neighbor)) {
                    ret = false;
                    break;
                }
                if (data.owner < data.neighbor) {
                    // The OpenFOAM spec requires:
                    // * An internal-face's normal points from the cell with the
                    //   lower index towards the cell with the higher index.
                    // * A boundary-face's normal points outside the owner cell.
                    //
                    // The GRDP spec requires:
                    // * An internal-face's normal points from the neighbor cell
                    //   towards the owner cell.
                    // * A boundary-face's normal points into the owner cell.
                    //
                    //               --- InteriorFaceNormal --->
                    //  OpenFOAM  Cell[LowNdx]        Cell[HighNdx]
                    //  GRDP API  Cell[NeighborNdx]   Cell[OwnerNdx]
                    //
                    //               --- BndryFaceNormal --->
                    //  OpenFOAM  Cell[OwnerNdx]   (GridExterior)
                    //  GRDP API  (GridExterior)   Cell[OwnerNdx]

                    // Since the OF owner index is < OF neighbor index, the face
                    // normal is wrong direction for PW. We could reverse the
                    // face vertices, but swapping the cell indices is faster.
                    std::swap(data.owner, data.neighbor);
                }
                // Add face to the assembler
                if (!PwAsmPushElementFace(hAsm, &data) ||
                        !grdpProgressIncr(&rti_)) {
                    ret = false;
                    break;
                }
            }
            // There should be one ) remaining and then EOF
            ret = ret && neighborFile_.wspaceSkipToChar(')') &&
                neighborFile_.wspaceCommentsSkip() &&
                neighborFile_.wspaceSkipToEOF();

            if (ret) {
                // The remaining faces are boundary (no neighbor)
                data.type = PWGM_FACETYPE_BOUNDARY;
                data.neighbor = PWP_UINT32_MAX;
                for (; ii < numFaces; ++ii) {
                    if (!readFaceVertices(data)) {
                        ret = false;
                        break;
                    }
                    if (!ownerFile_.readNextLabel(data.owner)) {
                        ret = false;
                        break;
                    }
                    // OF boundary faces always have the wrong face normal for
                    // PW. Need to reverse the face so the normal points INTO
                    // the owner cell.
                    reverseFace(data);

                    // Add face to the assembler
                    if (!PwAsmPushElementFace(hAsm, &data) ||
                            !grdpProgressIncr(&rti_)) {
                        ret = false;
                        break;
                    }
                }
                // There should be one ) remaining and then EOF
                ret = ret && ownerFile_.wspaceSkipToChar(')') &&
                    ownerFile_.wspaceCommentsSkip() &&
                    ownerFile_.wspaceSkipToEOF();
            }
        }
        // Stitch all the faces into cells
        return grdpProgressEndStep(&rti_) && ret && PwAsmFinalize(hAsm);
    }


    bool readFaceVertices(PWGM_ASSEMBLER_DATA &data)
    {
        bool ret = facesFile_.readNextFace(data);
        if (ret) {
            const PWP_UINT32 numPts = pointsFile_.getNumPts();
            // Check if any face vertex indices are out of range
            for (PWP_UINT32 jj = 0; jj < data.vertCnt; ++jj) {
                if (numPts <= data.index[jj]) {
                    ret = false;
                    break;
                }
            }
        }
        return ret;
    }


private:
    OpenFOAMGridReader(const OpenFOAMGridReader&);
    const OpenFOAMGridReader& operator=(const OpenFOAMGridReader&);


private:
    GRDP_RTITEM &       rti_;
    PWGM_HVERTEXLIST    hVL_;
    FaceListFile        facesFile_;
    LabelListFile       ownerFile_;
    LabelListFile       neighborFile_;
    VectorFieldFile     pointsFile_; 
};



PWP_BOOL
runtimeReadGrid(GRDP_RTITEM *pRti)
{
    OpenFOAMGridReader grid(*pRti);
    return grid.read();
}


PWP_BOOL
assignValueEnum(const char name[], const char value[],
    bool createIfNotExists)
{
    return PwuAssignValueEnum(GRDP_INFO_GROUP, name, value, createIfNotExists);
}


/** runtimeReadGridCreate() - (API function)
*
*   The plugin API entry point for plugin initialization,
*   called when the plugin is loaded.
*
*/
PWP_BOOL
runtimeReadGridCreate(GRDP_RTITEM *pRti)
{
    (void)pRti;
    PWP_BOOL ret = PWP_TRUE;

    // Element types supported by this importer
    const char *etypes = "Bar|Tri|Quad|Tet|Pyramid|Wedge|Hex";
    ret = ret && assignValueEnum("ValidElements", etypes, true);

    // A space delimited string of glob filters to identify filenames
    // supported by this importer.
    const char *filters = "faces owner neighbour points";
    ret = ret && assignValueEnum("FileFilters", filters, true);

    return ret;
}


/** runtimeReadGridDestroy() - (API function)
*
*   The plugin API entry point for plugin destruction,
*   called when the plugin is unloaded.
*
*/
PWP_VOID
runtimeReadGridDestroy(GRDP_RTITEM *pRti)
{
    (void)pRti;
}


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
