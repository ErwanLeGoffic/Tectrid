#ifndef _BRUSH_INFLATE_H_
#define _BRUSH_INFLATE_H_

#include "Brush.h"

class BrushInflate : public Brush
{
public:
	BrushInflate(Mesh& mesh) : Brush(mesh, BRUSHTYPE_INFLATE)
	{
		_mirroredBrush.reset(new BrushInflate(mesh, true));
	}

private:
	BrushInflate(Mesh& mesh, bool /*mirroredBrush*/) : Brush(mesh, BRUSHTYPE_INFLATE) { }

	virtual void DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio);
};

#endif // _BRUSH_INFLATE_H_
