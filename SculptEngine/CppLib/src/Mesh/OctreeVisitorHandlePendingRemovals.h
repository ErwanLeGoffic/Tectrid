#ifndef _OCTREE_VISITOR_HANDLEPENDINGREMOVALS_H_
#define _OCTREE_VISITOR_HANDLEPENDINGREMOVALS_H_

#include "OctreeVisitor.h"

class Mesh;

class OctreeVisitorHandlePendingRemovals : public OctreeVisitor
{
public:
	OctreeVisitorHandlePendingRemovals(Mesh const& mesh, bool doRemapping) : _mesh(mesh), _doRemapping(doRemapping), OctreeVisitor() {}
	virtual bool HasToVisit(OctreeCell& cell);
	virtual void VisitEnter(OctreeCell& cell);
	virtual void VisitLeave(OctreeCell& cell);

private:
	bool _doRemapping;	// Remapping of vertices and triangles is not necessary when we do recycling (i.e during all updates between start and end stroke)
	Mesh const& _mesh;
};

#endif // _OCTREE_VISITOR_HANDLEPENDINGREMOVALS_H_