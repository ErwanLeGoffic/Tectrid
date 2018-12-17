#ifndef _BRUSH_DRAW_H_
#define _BRUSH_DRAW_H_

#include "Brush.h"

class BrushDraw : public Brush
{
public:
	BrushDraw(Mesh& mesh) : Brush(mesh, BRUSHTYPE_DRAW)
	{
		_mirroredBrush.reset(new BrushDraw(mesh, true));
	}

private:
	BrushDraw(Mesh& mesh, bool /*mirroredBrush*/) : Brush(mesh, BRUSHTYPE_DRAW)	{ }

	virtual void DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio);
};

#endif // _BRUSH_DRAW_H_
