#ifndef _OCTREE_VISITOR_COLLECTBBOX_H_
#define _OCTREE_VISITOR_COLLECTBBOX_H_

#include <vector>
#include "OctreeVisitor.h"

class BBox;

class VisitorCollectBBox : public OctreeVisitor
{
public:
	VisitorCollectBBox() : OctreeVisitor() {}
	virtual bool HasToVisit(OctreeCell& cell);
	virtual void VisitEnter(OctreeCell& cell);
	virtual void VisitLeave(OctreeCell& /*cell*/) {}

	std::vector<BBox> const& GetBBoxes() const { return _BBoxes; }

private:
	std::vector<BBox> _BBoxes;
};

#endif // _OCTREE_VISITOR_COLLECTBBOX_H_