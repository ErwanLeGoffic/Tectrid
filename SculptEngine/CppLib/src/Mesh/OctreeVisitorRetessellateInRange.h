#ifndef _OCTREE_VISITOR_RETESSELLATEINRANGE_H_
#define _OCTREE_VISITOR_RETESSELLATEINRANGE_H_

#include <vector>
#include "OctreeVisitor.h"
#include "Math\Vector.h"
#include "Mesh.h"
#include "Loop.h"

class VisitorRetessellateInRange : public OctreeVisitor
{
public:
	VisitorRetessellateInRange(Mesh& mesh, float dDetail) : OctreeVisitor(), _mesh(mesh)
	{
		_mesh.GrabRetessellator().SetMaxEdgeLength(dDetail);
	}
	virtual void VisitEnter(OctreeCell& cell);
	virtual void VisitLeave(OctreeCell& /*cell*/) {}

private:
	virtual bool Intersects(BSphere const& /*sphere*/) = 0;
	void HandleCrushedTriangle(unsigned int triIdx);
	void HandleTriangleMerge(unsigned int triIdx);
	bool HandleTriangleSubdiv(unsigned int triIdx, bool testDistance, bool testLength);	// return false if it's not possible

	Mesh& _mesh;
};

class VisitorRetessellateInSphereRange: public VisitorRetessellateInRange
{
public:
	VisitorRetessellateInSphereRange(Mesh& mesh, Vector3 const& rangeCenterPoint, float rangeRadius, float brushStrengthRatio): VisitorRetessellateInRange(mesh, ComputDDetail(mesh, rangeRadius, brushStrengthRatio)), _rangeSphere(rangeCenterPoint, rangeRadius) { }

	virtual bool HasToVisit(OctreeCell& cell)
	{
		return _rangeSphere.Intersects(cell.GetContentBBox());
	}

	virtual bool Intersects(BSphere const& sphere)
	{
		return _rangeSphere.Intersects(sphere);
	}

private:
	float ComputDDetail(Mesh& mesh, float rangeRadius, float brushStrengthRatio)
	{
		BBox const& modelBbox = mesh.GetBBox();
		float modelRadius = modelBbox.Size().Length() * (0.5f / (float) M_SQRT2);
		float ratio = rangeRadius / modelRadius;
		float dDetail = rangeRadius / lerp(2.0f, 30.0f, ratio);	// Maximum edge length
		dDetail /= lerp(1.0f, 1.5f, brushStrengthRatio);	// When strength is high, have to subdivide more, or we'll see the triangles
#ifdef LOW_DEF
		dDetail *= 3.0f;
#elif defined(MEDIUM_DEF)
		dDetail *= 2.0f;
#endif // LOW_DEF / MEDIUM_DEF
		return dDetail;
	}

	BSphere _rangeSphere;
};

class VisitorRetessellateInLoopRange: public VisitorRetessellateInRange
{
public:
	VisitorRetessellateInLoopRange(Mesh& mesh, Loop const& loop, float range, float dDetail): VisitorRetessellateInRange(mesh, dDetail), _loop(loop), _range(range) { }

	virtual bool HasToVisit(OctreeCell& cell)
	{
		BBox cellPlusRange(cell.GetContentBBox());
		float boxRadius = cellPlusRange.Size().Length() / 2.0f;
		cellPlusRange.Scale((boxRadius + _range) / boxRadius);
		return _loop.Intersects(cellPlusRange);
	}

	virtual bool Intersects(BSphere const& sphere)
	{
		return _loop.Intersects(BSphere(sphere.GetCenter(), sphere.GetRadius() + _range));
	}

private:
	Loop const& _loop;
	float _range;
};

#endif // _OCTREE_VISITOR_RETESSELLATEINRANGE_H_