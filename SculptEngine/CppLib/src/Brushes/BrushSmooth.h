#ifndef _BRUSH_SMOOTH_H_
#define _BRUSH_SMOOTH_H_

#include "Brush.h"

class BrushSmooth : public Brush
{
public:
	BrushSmooth(Mesh& mesh) : Brush(mesh, BRUSHTYPE_SMOOTH)
	{
		_mirroredBrush.reset(new BrushSmooth(mesh, true));
	}

private:
	BrushSmooth(Mesh& mesh, bool /*mirroredBrush*/) : Brush(mesh, BRUSHTYPE_SMOOTH) { }
	virtual float GetEffectRadiusPercentForTimeAliasing() { return 0.0f; }	// Return 0.0 to cancel time aliasing

	virtual void DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio);
};

#endif // _BRUSH_SMOOTH_H_