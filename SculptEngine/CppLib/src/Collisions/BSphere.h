#ifndef _BSPHERE_H_
#define _BSPHERE_H_

#include "Math\Math.h"
#include "Math\Matrix.h"
#include <float.h>
#include "BBox.h"

class BSphere
{
public:
	BSphere(): _radius(-FLT_MAX) {}
	BSphere(Vector3 const& center, float radius) : _center(center), _radius(radius) {}
	BSphere(Vector3 const& triVtx1, Vector3 const& triVtx2, Vector3 const& triVtx3)
	{
		BuildFromTriangle(triVtx1, triVtx2, triVtx3);
	}
	
	void BuildFromTriangle(Vector3 const& triVtx1, Vector3 const& triVtx2, Vector3 const& triVtx3)
	{
		// Get triangle centroid
		_center = (triVtx1 + triVtx2 + triVtx3) / 3.0f;
		// Get triangle bounding sphere radius (centered on centroid)
		float squareDist1 = (triVtx1 - _center).LengthSquared();
		float squareDist2 = (triVtx2 - _center).LengthSquared();
		float squareDist3 = (triVtx3 - _center).LengthSquared();
		_radius = sqrtf(max(max(squareDist1, squareDist2), squareDist3));
	}

	bool IsValid() const { return _radius >= 0.0f; }

	Vector3 const& GetCenter() const { ASSERT(IsValid()); return _center; }
	float GetRadius() const { ASSERT(IsValid()); return _radius; }

	bool Intersects(Vector3 const& other, float epsilon) const
	{
		float squareDist = (other - GetCenter()).LengthSquared();
		return squareDist < sqr(GetRadius() + epsilon);
	}

	bool Intersects(BSphere const& other) const
	{
		float squareDist = (other.GetCenter() - GetCenter()).LengthSquared();
		return squareDist < sqr(GetRadius() + other.GetRadius());
	}

	// Found here: https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
	bool Intersects(BBox const& box) const
	{
		// get box closest point to sphere center by clamping
		float x = max(box.Min().x, min(GetCenter().x, box.Max().x));
		float y = max(box.Min().y, min(GetCenter().y, box.Max().y));
		float z = max(box.Min().z, min(GetCenter().z, box.Max().z));

		// this is the same as isPointInsideSphere
		float squareDistance = (x - GetCenter().x) * (x - GetCenter().x) +
			(y - GetCenter().y) * (y - GetCenter().y) +
			(z - GetCenter().z) * (z - GetCenter().z);

		return squareDistance < sqr(GetRadius());
	}

	void Transform(Matrix3 const& rotAndScale, Vector3 const& position)
	{
		rotAndScale.Transform(_center);
		_center += position;
		Vector3 radius3(_radius, _radius, _radius);
		rotAndScale.Transform(radius3);
		_radius = max(max(radius3.x, radius3.y), radius3.z);
	}

private:
	Vector3 _center;
	float _radius;
};

#endif // _BSPHERE_H_