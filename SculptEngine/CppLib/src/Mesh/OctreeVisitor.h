#ifndef _OCTREE_VISITOR_H_
#define _OCTREE_VISITOR_H_

class OctreeCell;

class OctreeVisitor
{
public:
	OctreeVisitor() {}
	virtual bool HasToVisit(OctreeCell& /*cell*/) = 0;
	virtual void VisitEnter(OctreeCell& /*cell*/) = 0; 
	virtual void VisitLeave(OctreeCell& /*cell*/) = 0;
};

#endif // _OCTREE_VISITOR_H_