#ifndef _PLANE_H_
#define _PLANE_H_

#include "Vector.h"

class Plane
{
public:
	Plane(Vector3 const& point, Vector3 const &normal)
	{
		_n = normal;
		_d = point.Dot(_n);
	}

	Plane(Vector3 const& a, Vector3 const& b, Vector3 const& c)
	{
		_n = (b - a).Cross(c - a);
		_n.Normalize();
		_d = a.Dot(_n);
	}

	float DistanceToVertex(Vector3 const& vtx) const
	{
		return _n.Dot(vtx) - _d;
	}

	Vector3 const& GetNormal() const { return _n; }

	void Flip()
	{
		_n.Negate();
		_d = -_d;
	}

protected:
	Vector3 _n;
	float _d;
};

#endif // _PLANE_H_