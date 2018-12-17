#ifndef _BRUSH_SMEAR_H_
#define _BRUSH_SMEAR_H_

#include "Brush.h"
#include "Mesh\OctreeVisitor.h"

class BrushSmear : public Brush
{
	class Visitor : public OctreeVisitor
	{
	public:
		Visitor(Mesh& fullMesh, Vector3 const& sourcePoint, float radius, float strengthRatio, Vector3 const& effectDisplacement) : OctreeVisitor(), _fullMesh(fullMesh), _rangeSphere(sourcePoint, radius), _effectDisplacement(effectDisplacement)
		{
			_radiusSquared = sqr(_rangeSphere.GetRadius());
			_invRadius = 1.0f / _rangeSphere.GetRadius();
			_smearRatio = lerp(0.005f, 0.05f, strengthRatio);
		}
		virtual bool HasToVisit(OctreeCell& cell);
		virtual void VisitEnter(OctreeCell& cell);
		virtual void VisitLeave(OctreeCell& /*cell*/) {}

	private:
		Mesh& _fullMesh;
		BSphere _rangeSphere;
		float _radiusSquared;
		Vector3 const _effectDisplacement;
		float _invRadius;
		float _smearRatio;
	};

public:
	BrushSmear(Mesh& mesh) : Brush(mesh, BRUSHTYPE_SMEAR)
	{
		_mirroredBrush.reset(new BrushSmear(mesh, true));
	}

private:
	BrushSmear(Mesh& mesh, bool /*mirroredBrush*/) : Brush(mesh, BRUSHTYPE_SMEAR) { }

	virtual void DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio);
	virtual float GetEffectRadiusPercentForTimeAliasing() { return 0.0f; }	// Return 0.0 to cancel time aliasing
};

#endif // _BRUSH_SMEAR_H_