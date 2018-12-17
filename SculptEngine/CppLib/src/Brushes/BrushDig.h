#ifndef _BRUSH_DIG_H_
#define _BRUSH_DIG_H_

#include "Brush.h"

class BrushDig : public Brush
{
public:
	BrushDig(Mesh& mesh) : Brush(mesh, BRUSHTYPE_DIG)
	{
		_mirroredBrush.reset(new BrushDig(mesh, true));
	}

private:
	BrushDig(Mesh& mesh, bool /*mirroredBrush*/) : Brush(mesh, BRUSHTYPE_DIG) { }

	virtual void DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio);
};

#endif // _BRUSH_DIG_H_
