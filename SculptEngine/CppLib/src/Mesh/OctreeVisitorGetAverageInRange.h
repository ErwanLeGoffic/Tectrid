#ifndef _OCTREE_VISITOR_GETAVERAGEINRANGE_H_
#define _OCTREE_VISITOR_GETAVERAGEINRANGE_H_

#include "OctreeVisitor.h"
#include "Math\Vector.h"
#include "Collisions\BSphere.h"

class Mesh;

class VisitorGetAverageInRange : public OctreeVisitor
{
public:
	VisitorGetAverageInRange(Mesh const& mesh, Vector3 const& rangeCenterPoint, float rangeRadius) : OctreeVisitor(), _mesh(mesh), _rangeSphere(rangeCenterPoint, rangeRadius), _rangeRadiusSquared(rangeRadius * rangeRadius), _nbVerticesAccumulated(0) {}
	virtual bool HasToVisit(OctreeCell& cell);
	virtual void VisitEnter(OctreeCell& cell);
	virtual void VisitLeave(OctreeCell& /*cell*/) {}

	Vector3 GetAveragePoint() const { return _verticesAccumulator / float(_nbVerticesAccumulated); }
	Vector3 GetAverageNormal() const { return _nbVerticesAccumulated == 0 ? Vector3(0, 1, 0) : (_normalsAccumulator / float(_nbVerticesAccumulated)).Normalized(); }

private:
	Mesh const& _mesh;
	BSphere _rangeSphere;
	float _rangeRadiusSquared;
	Vector3 _verticesAccumulator;
	Vector3 _normalsAccumulator;
	unsigned int _nbVerticesAccumulated;
};

#endif // _OCTREE_VISITOR_GETAVERAGEINRANGE_H_