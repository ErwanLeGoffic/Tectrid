#ifndef _BRUSH_DRAG_H_
#define _BRUSH_DRAG_H_

#include "Brush.h"

class Plane;

class BrushDrag : public Brush
{
public:
	BrushDrag(Mesh& mesh) : Brush(mesh, BRUSHTYPE_DRAG), _dragDistance(0.0f)
	{
		_mirroredBrush.reset(new BrushDrag(mesh, true));
	}

	virtual void UpdateStroke(Ray const& ray, float radius, float strengthRatio);

#ifdef BRUSHES_DEBUG_DRAW
	Vector3 const& GetProjectionCenter() { return _projectionSphereCenter; }
	Vector3 const& GetTheoricalDestinationPoint() { return _theoricalDestinationPoint; }
	Vector3 const& GetRefDraggedPoint() { return _refDraggedPoint; }
#endif // BRUSHES_DEBUG_DRAW

private:
	BrushDrag(Mesh& mesh, bool /*mirroredBrush*/) : Brush(mesh, BRUSHTYPE_DRAG), _dragDistance(0.0f) { }

	//virtual float GetEffectRadiusPercentForTimeAliasing() { return 0.0f; }	// Return 0.0 to cancel time aliasing

	virtual void DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio);

	float ComputeRadiusRatioFromAttractorDist(float radius);

	Vector3 _projectionSphereCenter;
	Vector3 _theoricalDestinationPoint;
	Vector3 _refDraggedPoint;
	float _dragDistance;
	unsigned int _nbDoStroke;
};

#endif // _BRUSH_DRAG_H_