#ifndef _OCTREE_VISITOR_EXTRACTOUTOFCELLSBOUNDGEOM_H_
#define _OCTREE_VISITOR_EXTRACTOUTOFCELLSBOUNDGEOM_H_

#include <vector>
#include "OctreeVisitor.h"

class Mesh;

class VisitorExtractOutOfCellsBoundGeom : public OctreeVisitor
{
public:
	VisitorExtractOutOfCellsBoundGeom(Mesh const& mesh, bool extractFromAllCells) : _mesh(mesh), _extractFromAllCells(extractFromAllCells), OctreeVisitor() {}
	virtual bool HasToVisit(OctreeCell& cell);
	virtual void VisitEnter(OctreeCell& cell);
	virtual void VisitLeave(OctreeCell& /*cell*/) {}

	std::vector<unsigned int> const& GetExtractedTris() { return _extractedTris; }
	std::vector<unsigned int> const& GetExtractedVtxs() { return _extractedVtxs; }

private:
	Mesh const& _mesh;
	bool _extractFromAllCells;
	std::vector<unsigned int> _extractedTris;
	std::vector<unsigned int> _extractedVtxs;
};

#endif // _OCTREE_VISITOR_EXTRACTOUTOFCELLSBOUNDGEOM_H_