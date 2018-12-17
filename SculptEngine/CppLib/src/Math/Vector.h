#ifndef _VECTOR_H_
#define _VECTOR_H_

#include "Math.h"
#include "SculptEngine.h"

class Vector3
{
public:
	Vector3(): x(0.0f), y(0.0f), z(0.0f) {}
	Vector3(float x, float y, float z): x(x), y(y), z(z) {}
	Vector3(Vector3 const& v) : x(v.x), y(v.y), z(v.z) {}

	Vector3 operator*(const float value) const
	{
		return Vector3(x * value, y * value, z * value);
	}

	Vector3 operator/(const float value) const
	{
		return Vector3(x / value, y / value, z / value);
	}

	void operator=(Vector3 const& v)
	{
		x = v.x; y = v.y; z = v.z;
	}

	Vector3 operator-()
	{
		return Vector3(-x, -y, -z);
	}

	Vector3 operator+(Vector3 const& v) const
	{
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	Vector3 operator-(Vector3 const& v) const
	{
		return Vector3(x - v.x, y - v.y, z - v.z);
	}

	void operator*=(float v)
	{
		x *= v;
		y *= v;
		z *= v;
	}

	void operator/=(float v)
	{
		ASSERT(v != 0.0f);
		x /= v;
		y /= v;
		z /= v;
	}

	void operator+=(Vector3 const& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
	}

	void operator-=(Vector3 const& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
	}

	void operator*=(Vector3 const& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
	}

	float Dot(Vector3 const& v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	Vector3	Cross(Vector3 const& v) const
	{
		return Vector3(	y * v.z - z * v.y, 
						z * v.x - x * v.z,
						x * v.y - y * v.x);
	}

	Vector3 Lerp(Vector3 const& a, float t) const
	{
		Vector3 result(a);
		result -= *this;
		result *= t;
		result += *this;
		return result;
	};

	void Normalize()
	{
		ASSERT(LengthSquared() > 0.0f);
		float length = Length();
		if(length != 0.0f)
			*this /= length;
		else
			this->x = 1.0f;
	}

	Vector3 Normalized() const
	{
		Vector3 n(*this);
		n.Normalize();
		return n;
	}

	float LengthSquared() const
	{
		return Dot(*this);
	}

	float Length() const
	{
		return sqrtf(LengthSquared());
	}

	float Distance(Vector3 const& p) const
	{
		return (*this - p).Length();
	}
	
	float DistanceSquared(Vector3 const& p) const
	{
		return (*this - p).LengthSquared();
	}

	void Negate()
	{
		x = -x; y = -y; z = -z;
	}

	bool operator<(Vector3 const& other) const
	{
		return x < other.x
			|| (x == other.x && (y < other.y
			|| (y == other.y && z < other.z)));
		/*const float eps = 1e-4f;
		return x < (other.x - eps)
			|| ((fabs(x - other.x) < eps) && (y < (other.y - eps)
			|| ((fabs(y - other.y) < eps) && z < (other.z - eps))));*/
	}

	bool operator==(Vector3 const& other) const
	{
		return (x == other.x) && (y == other.y) && (z == other.z);
	}

	bool operator!=(Vector3 const& other) const
	{
		return (x != other.x) || (y != other.y) || (z != other.z);
	}

	void ResetToZero()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

#ifdef __EMSCRIPTEN__
	float X() { return x; }
	float Y() { return y; }
	float Z() { return z; }
#endif // __EMSCRIPTEN__

public:
	float x, y, z;
};

#endif // _VECTOR_H_