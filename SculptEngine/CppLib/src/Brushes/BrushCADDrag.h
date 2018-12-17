#ifndef _BRUSH_CADDRAG_H_
#define _BRUSH_CADDRAG_H_

#include "Brush.h"
#include <set>

class Plane;

class BrushCADDrag : public Brush
{
public:
	BrushCADDrag(Mesh& mesh): Brush(mesh, BRUSHTYPE_CADDRAG), _dragDistance(0.0f)
	{
		_mirroredBrush.reset(new BrushCADDrag(mesh, true));
	}

	virtual void UpdateStroke(Ray const& ray, float radius, float strengthRatio);
	virtual void EndStroke();

private:
	BrushCADDrag(Mesh& mesh, bool /*mirroredBrush*/): Brush(mesh, BRUSHTYPE_CADDRAG), _dragDistance(0.0f) {}

	virtual float GetEffectRadiusPercentForTimeAliasing() { return 0.0f; }	// Return 0.0 to cancel time aliasing
	virtual void DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio);

	void SelectFace(unsigned int inputTriIdx, std::vector<unsigned int>& outputSelectedTris);

	float _dragDistance;
	std::vector<unsigned int> _faceTriangles;
	std::set<unsigned int> _faceVertices;
	Vector3 _initialDragPoint;
	Vector3 _initialDragNormal;
};

#endif // _BRUSH_CADDRAG_H_