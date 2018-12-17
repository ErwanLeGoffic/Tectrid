#ifndef _RAY_H_
#define _RAY_H_

#include "Math\Math.h"
#include "BBox.h"
#include "BSphere.h"

class Ray
{
public:
	Ray() : _length(0.0f) {}
	Ray(Vector3 const& origin, Vector3 const& direction, float length) : _origin(origin), _direction(direction), _length(length) {}

	Vector3 const& GetOrigin() const { return _origin; }
	Vector3 const& GetDirection() const { return _direction; }
	Vector3& GrabOrigin() { return _origin; }
	Vector3& GrabDirection() { return _direction; }
	float GetLength() const { return _length; }
	bool IsValid() const { return _length != 0.0f; }
	void Invalidate() { _length = 0.0f; }
	void SetOrigin(Vector3 const& origin) { _origin = origin; }
	void SetDirection(Vector3 const& direction) { _direction = direction; }
	void SetLength(float length) { _length = length; }

	// Found here: http://gamedev.stackexchange.com/questions/18436/most-efficient-aabb-vs-ray-collision-algorithms/
	bool Intersects(BBox const& b) const
	{
		// r.dir is unit direction vector of ray
		Vector3 dirfrac((GetDirection().x != 0.0f) ? 1.0f / GetDirection().x : 1000000.0f,
			(GetDirection().y != 0.0f) ? 1.0f / GetDirection().y : 1000000.0f,
			(GetDirection().z != 0.0f) ? 1.0f / GetDirection().z : 1000000.0f);
		// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
		// r.org is origin of ray
		float t1 = (b.Min().x - GetOrigin().x) * dirfrac.x;
		float t2 = (b.Max().x - GetOrigin().x) * dirfrac.x;
		float t3 = (b.Min().y - GetOrigin().y) * dirfrac.y;
		float t4 = (b.Max().y - GetOrigin().y) * dirfrac.y;
		float t5 = (b.Min().z - GetOrigin().z) * dirfrac.z;
		float t6 = (b.Max().z - GetOrigin().z) * dirfrac.z;

		float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
		float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

		// if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
		if(tmax < 0.0f)
		{
			//colDist = tmax;
			return false;
		}

		// if tmin > tmax, ray doesn't intersect AABB
		if(tmin > tmax)
		{
			//colDist = tmax;
			return false;
		}

		//colDist = tmin;
		if(tmin <= GetLength())
			return true;
		else
			return false;	// Collide, but too far
	}

	bool Intersects(BSphere const& sphere) const
	{
		// Explanation here : http://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection
		Vector3 L = sphere.GetCenter() - GetOrigin();
		float tca = L.Dot(GetDirection());
		if(tca < 0)
			return false;
		float d2 = L.LengthSquared() - sqr(tca);
		float squaredRadius = sqr(sphere.GetRadius());
		if(d2 > squaredRadius)
			return false;	// Wrong direction
		float thc = sqrtf(squaredRadius - d2);
		if(_length < (tca - thc))
			return false;	// Wrong distance
		return true;
	}

	// Möller–Trumbore intersection algorithm
	// Found here: https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
	bool Intersects(Vector3 const& V1, // Triangle vertices 
					Vector3 const& V2,
					Vector3 const& V3,
					float& distance,
					bool cullBackFace) const
	{
		Vector3 e1, e2;  // Edge1, Edge2
		Vector3 P, Q, T;
		float det, inv_det, u, v;
		float t;
		distance = -1.0f;

		// Find vectors for two edges sharing V1
		e1 = V2 - V1;
		e2 = V3 - V1;
		// Begin calculating determinant - also used to calculate u parameter
		P = GetDirection().Cross(e2);
		// If determinant is near zero, ray lies in plane of triangle or ray is parallel to plane of triangle
		det = e1.Dot(P);
		if(cullBackFace)
		{
			if((SculptEngine::IsTriangleOrientationInverted() && (det > -EPSILON))
				|| (!SculptEngine::IsTriangleOrientationInverted() && (det < EPSILON)))
				return false;
		}
		else
		{
			if(det > -EPSILON && det < EPSILON)
				return false;
		}
		inv_det = 1.0f / det;

		// Calculate distance from V1 to ray origin
		T = GetOrigin() - V1;

		// Calculate u parameter and test bound
		u = T.Dot(P) * inv_det;
		// The intersection lies outside of the triangle
		if((u < 0.0f) || (u > 1.0f))
			return false;

		// Prepare to test v parameter
		Q = T.Cross(e1);

		// Calculate V parameter and test bound
		v = GetDirection().Dot(Q) * inv_det;
		// The intersection lies outside of the triangle
		if((v < 0.0f) || (u + v > 1.0f))
			return false;

		t = e2.Dot(Q) * inv_det;

		if(t > EPSILON)
		{ // Ray intersect
			if(t <= GetLength())
			{
				distance = t;
				return true;
			}
			else
				return false;	// Collide, but too far
		}

		// No hit, no win
		return false;
	}

private:
	Vector3 _origin;
	Vector3 _direction;
	float _length;
};

#endif // _RAY_H_