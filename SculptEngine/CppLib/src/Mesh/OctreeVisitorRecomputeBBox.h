#ifndef _OCTREE_VISITOR_RECOMPUTEBBOX_H_
#define _OCTREE_VISITOR_RECOMPUTEBBOX_H_

#include "OctreeVisitor.h"

class Mesh;

class VisitorRecomputeBBox : public OctreeVisitor
{
public:
	VisitorRecomputeBBox(Mesh const& mesh, bool forceAllCellRecomputing) : OctreeVisitor(), _mesh(mesh), _forceAllCellRecomputing(forceAllCellRecomputing) {}
	virtual bool HasToVisit(OctreeCell& cell);
	virtual void VisitEnter(OctreeCell& /*cell*/) {}
	virtual void VisitLeave(OctreeCell& cell);

private:
	Mesh const& _mesh;
	bool _forceAllCellRecomputing;
};

#endif // _OCTREE_VISITOR_RECOMPUTEBBOX_H_