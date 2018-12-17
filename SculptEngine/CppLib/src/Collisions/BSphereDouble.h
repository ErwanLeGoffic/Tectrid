#ifndef _BSPHEREDOUBLE_H_
#define _BSPHEREDOUBLE_H_

#include "Math\Math.h"
#include "Math\PlaneDouble.h"
#include <float.h>

// Todo: use template
class BSphereDouble
{
public:
	BSphereDouble(): _radius(-DBL_MAX) {}
	BSphereDouble(Vector3Double const& center, double radius) : _center(center), _radius(radius) {}
	BSphereDouble(Vector3Double const& triVtx1, Vector3Double const& triVtx2, Vector3Double const& triVtx3)
	{
		BuildFromTriangle(triVtx1, triVtx2, triVtx3);
	}
	
	void BuildFromTriangle(Vector3Double const& triVtx1, Vector3Double const& triVtx2, Vector3Double const& triVtx3)
	{
		// Get triangle centroid
		_center = (triVtx1 + triVtx2 + triVtx3) / 3.0f;
		// Get triangle bounding sphere radius (centered on centroid)
		double squareDist1 = (triVtx1 - _center).LengthSquared();
		double squareDist2 = (triVtx2 - _center).LengthSquared();
		double squareDist3 = (triVtx3 - _center).LengthSquared();
		_radius = sqrt(max(max(squareDist1, squareDist2), squareDist3));
	}

	bool IsValid() const { return _radius >= 0.0; }

	Vector3Double const& GetCenter() const { ASSERT(IsValid()); return _center; }
	double GetRadius() const { ASSERT(IsValid()); return _radius; }

	bool Intersects(Vector3Double const& other, double epsilon) const
	{
		double squareDist = (other - GetCenter()).LengthSquared();
		return squareDist < sqr(GetRadius() + epsilon);
	}

	bool Intersects(BSphereDouble const& other) const
	{
		double squareDist = (other.GetCenter() - GetCenter()).LengthSquared();
		return squareDist < sqr(GetRadius() + other.GetRadius());
	}

private:
	Vector3Double _center;
	double _radius;
};

#endif // _BSPHEREDOUBLE_H_