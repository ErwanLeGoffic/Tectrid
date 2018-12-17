#ifndef _OCTREE_VISITOR_COLLECTVERTICES_H_
#define _OCTREE_VISITOR_COLLECTVERTICES_H_

#include "OctreeVisitor.h"
#include "Math\Vector.h"
#include "Collisions\BSphere.h"
#include <vector>

class Mesh;
class ThicknessHandler;

class OctreeVisitorCollectVertices : public OctreeVisitor
{
public:
	OctreeVisitorCollectVertices(Mesh const& mesh, Vector3 const& rangeCenterPoint, float rangeRadius, Vector3 const* selectDirection, float cosAngleLimit, bool dirtyOctreeCells, bool collectRejected, ThicknessHandler& thicknessHandler);
	virtual bool HasToVisit(OctreeCell& cell);
	virtual void VisitEnter(OctreeCell& cell);
	virtual void VisitLeave(OctreeCell& /*cell*/) {}

	std::vector<unsigned int> const& GetVertices() const { return _collectedVertices; }
	std::vector<unsigned int> const& GetRejectedVertices() const { return _rejectedVertices; }
	Vector3 ComputeAveragePosition() const;
	Vector3 ComputeAverageNormal() const;

private:
	Mesh const& _mesh;
	BSphere _rangeSphere;
	Vector3 const * const _selectDirection;
	float _rangeRadiusSquared;
	float _cosAngleLimit;
	std::vector<unsigned int> _collectedVertices;
	std::vector<unsigned int> _rejectedVertices;
	bool _dirtyOctreeCells;
	bool _collectRejected;
	ThicknessHandler& _thicknessHandler;
};

#endif // _OCTREE_VISITOR_COLLECTVERTICES_H_