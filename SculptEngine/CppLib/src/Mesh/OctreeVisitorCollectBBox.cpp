#include "OctreeVisitorCollectBBox.h"
#include "Octree.h"
#include "Collisions\BBox.h"

bool VisitorCollectBBox::HasToVisit(OctreeCell& /*cell*/)
{
	return true;
}

void VisitorCollectBBox::VisitEnter(OctreeCell& cell)
{
	if(!cell.GetTrianglesIdx().empty() || !cell.GetVerticesIdx().empty())
		_BBoxes.push_back(cell.GetContentBBox());
}
