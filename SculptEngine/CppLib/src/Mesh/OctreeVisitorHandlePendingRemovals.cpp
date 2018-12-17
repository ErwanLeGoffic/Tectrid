#include "OctreeVisitorHandlePendingRemovals.h"
#include "Octree.h"

bool OctreeVisitorHandlePendingRemovals::HasToVisit(OctreeCell& /*cell*/)
{
	return true;
}

void OctreeVisitorHandlePendingRemovals::VisitEnter(OctreeCell& cell)
{
	if(!cell.GetTrianglesIdx().empty() || !cell.GetVerticesIdx().empty())
		cell.HandlePendingRemovals(_mesh, _doRemapping);
}

void OctreeVisitorHandlePendingRemovals::VisitLeave(OctreeCell& cell)
{
	cell.PurgeEmptyChildren();
}