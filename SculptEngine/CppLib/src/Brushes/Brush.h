#ifndef _BRUSH_H_
#define _BRUSH_H_

#include "Mesh\Mesh.h"
#include "Math\Math.h"

//#define DEBUG_BRUSHES

enum BRUSHTYPE: unsigned char
{
	BRUSHTYPE_DRAW,
	BRUSHTYPE_INFLATE,
	BRUSHTYPE_FLATTEN,
	BRUSHTYPE_SMEAR,
	BRUSHTYPE_DRAG,
	BRUSHTYPE_DIG,
	BRUSHTYPE_SMOOTH,
	BRUSHTYPE_CADDRAG
};

class Brush
{
public:
	Brush(Mesh& mesh, BRUSHTYPE type): _mesh(mesh), _type(type), _strokeStarted(false) {}

	void StartStroke();
	virtual void UpdateStroke(Ray const& ray, float radius, float strengthRatio);
	virtual void EndStroke();

#ifdef BRUSHES_DEBUG_DRAW
	Vector3 const& GetLastIntersection() { return _lastIntersectionPos; }
#endif // BRUSHES_DEBUG_DRAW

protected:
	virtual void DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio);

private:
	virtual float GetEffectRadiusPercentForTimeAliasing() { return 0.3f; }	// Return 0.0 to cancel time aliasing
	
protected:
	Mesh& _mesh;
	Ray _lastRay;
	Ray _curRay;
	Vector3 _lastIntersectionPos;
	Vector3 _motionDir;
	std::unique_ptr<Brush> _mirroredBrush;
	BRUSHTYPE _type;

private:
	bool _strokeStarted;
};

#endif // _BRUSH_H_