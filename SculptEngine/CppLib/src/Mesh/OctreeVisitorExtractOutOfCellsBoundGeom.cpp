#include "OctreeVisitorExtractOutOfCellsBoundGeom.h"
#include "Octree.h"

bool VisitorExtractOutOfCellsBoundGeom::HasToVisit(OctreeCell& /*cell*/)
{
	return true;
}

void VisitorExtractOutOfCellsBoundGeom::VisitEnter(OctreeCell& cell)
{
	if(!cell.GetTrianglesIdx().empty() || !cell.GetVerticesIdx().empty())
	{
		if(!_extractFromAllCells && !cell.TestStateFlags(CELL_STATE_HASTO_EXTRACT_OUTOFBOUNDS_GEOM))
			return;
		cell.ClearStateFlags(CELL_STATE_HASTO_EXTRACT_OUTOFBOUNDS_GEOM);
		cell.ExtractOutOfBoundsGeom(_mesh, _extractedTris, _extractedVtxs);
	}
}
