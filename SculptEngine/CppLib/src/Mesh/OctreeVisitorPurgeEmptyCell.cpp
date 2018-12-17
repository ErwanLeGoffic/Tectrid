#include "OctreeVisitorPurgeEmptyCell.h"
#include "Octree.h"

bool VisitorPurgeEmptyCell::HasToVisit(OctreeCell& /*cell*/)
{
	return true;
}

void VisitorPurgeEmptyCell::VisitLeave(OctreeCell& cell)
{
	cell.PurgeEmptyChildren();
}
