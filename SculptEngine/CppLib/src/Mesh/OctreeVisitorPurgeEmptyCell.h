#ifndef _OCTREE_VISITOR_PURGEEMPTYCELL_H_
#define _OCTREE_VISITOR_PURGEEMPTYCELL_H_

#include "OctreeVisitor.h"

class VisitorPurgeEmptyCell : public OctreeVisitor
{
public:
	VisitorPurgeEmptyCell() : OctreeVisitor() {}
	virtual bool HasToVisit(OctreeCell& cell);
	virtual void VisitEnter(OctreeCell& /*cell*/) {}
	virtual void VisitLeave(OctreeCell& cell);
};

#endif // _OCTREE_VISITOR_PURGEEMPTYCELL_H_