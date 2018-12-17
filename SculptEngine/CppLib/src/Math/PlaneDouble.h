#ifndef _PLANEDOUBLE_H_
#define _PLANEDOUBLE_H_

#include "VectorDouble.h"

// Todo: use template
class PlaneDouble
{
public:
	PlaneDouble(Vector3Double const& point, Vector3Double const &normal)
	{
		_n = normal;
		_d = point.Dot(_n);
	}

	PlaneDouble(Vector3Double const& a, Vector3Double const& b, Vector3Double const& c)
	{
		_n = (b - a).Cross(c - a);
		_n.Normalize();
		_d = a.Dot(_n);
	}

	double DistanceToVertex(Vector3Double const& vtx) const
	{
		return _n.Dot(vtx) - _d;
	}

	Vector3Double const& GetNormal() const { return _n; }

	void Flip()
	{
		_n.Negate();
		_d = -_d;
	}

protected:
	Vector3Double _n;
	double _d;
};

#endif // _PLANEDOUBLE_H_