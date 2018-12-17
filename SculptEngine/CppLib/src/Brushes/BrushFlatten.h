#ifndef _BRUSH_FLATTEN_H_
#define _BRUSH_FLATTEN_H_

#include "Brush.h"

class Plane;

class BrushFlatten : public Brush
{
public:
	BrushFlatten(Mesh& mesh) : Brush(mesh, BRUSHTYPE_FLATTEN)
	{
		_mirroredBrush.reset(new BrushFlatten(mesh, true));
	}

private:
	BrushFlatten(Mesh& mesh, bool /*mirroredBrush*/) : Brush(mesh, BRUSHTYPE_FLATTEN) { }

	virtual void DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio);
};

#endif // _BRUSH_FLATTEN_H_
