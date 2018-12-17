#ifndef _VECTORDOUBLE_H_
#define _VECTORDOUBLE_H_

#include "Math.h"
#include "SculptEngine.h"

// Todo: use template
class Vector3Double
{
public:
	Vector3Double(): x(0.0), y(0.0), z(0.0) {}
	Vector3Double(double x, double y, double z): x(x), y(y), z(z) {}
	Vector3Double(Vector3Double const& v): x(v.x), y(v.y), z(v.z) {}

	Vector3Double operator*(const double value) const
	{
		return Vector3Double(x * value, y * value, z * value);
	}

	Vector3Double operator/(const double value) const
	{
		return Vector3Double(x / value, y / value, z / value);
	}

	void operator=(Vector3Double const& v)
	{
		x = v.x; y = v.y; z = v.z;
	}

	Vector3Double operator-()
	{
		return Vector3Double(-x, -y, -z);
	}

	Vector3Double operator+(Vector3Double const& v) const
	{
		return Vector3Double(x + v.x, y + v.y, z + v.z);
	}

	Vector3Double operator-(Vector3Double const& v) const
	{
		return Vector3Double(x - v.x, y - v.y, z - v.z);
	}

	void operator*=(double v)
	{
		x *= v;
		y *= v;
		z *= v;
	}

	void operator/=(double v)
	{
		ASSERT(v != 0.0);
		x /= v;
		y /= v;
		z /= v;
	}

	void operator+=(Vector3Double const& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
	}

	void operator-=(Vector3Double const& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
	}

	void operator*=(Vector3Double const& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
	}

	double Dot(Vector3Double const& v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	Vector3Double Cross(Vector3Double const& v) const
	{
		return Vector3Double(y * v.z - z * v.y,
			z * v.x - x * v.z,
			x * v.y - y * v.x);
	}

	Vector3Double Lerp(Vector3Double const& a, double t) const
	{
		Vector3Double result(a);
		result -= *this;
		result *= t;
		result += *this;
		return result;
	};

	void Normalize()
	{
		ASSERT(LengthSquared() > 0.0);
		double length = Length();
		if(length != 0.0)
			*this /= length;
		else
			this->x = 1.0;
	}

	Vector3Double Normalized() const
	{
		Vector3Double n(*this);
		n.Normalize();
		return n;
	}

	double LengthSquared() const
	{
		return Dot(*this);
	}

	double Length() const
	{
		return sqrt(LengthSquared());
	}

	double Distance(Vector3Double const& p) const
	{
		return (*this - p).Length();
	}

	double DistanceSquared(Vector3Double const& p) const
	{
		return (*this - p).LengthSquared();
	}

	void Negate()
	{
		x = -x; y = -y; z = -z;
	}

	bool operator<(Vector3Double const& other) const
	{
		return x < other.x
			|| (x == other.x && (y < other.y
			|| (y == other.y && z < other.z)));
		/*const float eps = 1e-3;
		return x < (other.x - eps)
		|| ((fabs(x - other.x) < eps) && (y < (other.y - eps)
		|| ((fabs(y - other.y) < eps) && z < (other.z - eps))));*/
	}

	void ResetToZero()
	{
		x = 0.0;
		y = 0.0;
		z = 0.0;
	}

public:
	double x, y, z;
};

#endif // _VECTORDOUBLE_H_