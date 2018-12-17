#ifndef _MATRIX_H_
#define _MATRIX_H_

#include "Vector.h"
#include <float.h>

class Matrix3
{
public:
	Matrix3(): Matrix3(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f))
	{
	}

	Matrix3(Vector3 const& right, Vector3 const& up, Vector3 const& front): _columns{ right, up, front }
	{	
	}
	
	Matrix3(float* mat3x3): _columns{ Vector3(mat3x3[0], mat3x3[1], mat3x3[2]), Vector3(mat3x3[3], mat3x3[4], mat3x3[5]), Vector3(mat3x3[6], mat3x3[7], mat3x3[8]) }
	{
	}

	void Transform(Vector3& v) const
	{
		Vector3 temp(v);
		v.x = temp.Dot(_columns[0]);
		v.y = temp.Dot(_columns[1]);
		v.z = temp.Dot(_columns[2]);
	}

	void operator*=(Matrix3& m)
	{
		Matrix3 temp(m);

		_columns[0].x = temp._columns[0].Dot(m._columns[0]);
		_columns[0].y = temp._columns[1].Dot(m._columns[0]);
		_columns[0].z = temp._columns[2].Dot(m._columns[0]);
		_columns[1].x = temp._columns[0].Dot(m._columns[1]);
		_columns[1].y = temp._columns[1].Dot(m._columns[1]);
		_columns[1].z = temp._columns[2].Dot(m._columns[1]);
		_columns[2].x = temp._columns[0].Dot(m._columns[2]);
		_columns[2].y = temp._columns[1].Dot(m._columns[2]);
		_columns[2].z = temp._columns[2].Dot(m._columns[2]);
	}

	void Invert()
	{
		auto m = [&](unsigned int a, unsigned int b) -> float
		{
			if((a < 3) && (b < 3))
			{
				switch(b)
				{
				case 0:
					return _columns[a].x;
					break;
				case 1:
					return _columns[a].y;
					break;
				case 2:
					return _columns[a].z;
					break;
				}
			}
			return FLT_MAX;
		};
		// computes the inverse of a matrix m
		float det = m(0, 0) * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) -
					m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
					m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));

		float invdet = 1.0f / det;

		Matrix3 minv; // inverse of matrix m
		minv._columns[0].x = +(m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) * invdet;
		minv._columns[0].y = -(m(0, 1) * m(2, 2) - m(0, 2) * m(2, 1)) * invdet;
		minv._columns[0].z = +(m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * invdet;
		minv._columns[1].x = -(m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) * invdet;
		minv._columns[1].y = +(m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0)) * invdet;
		minv._columns[1].z = -(m(0, 0) * m(1, 2) - m(1, 0) * m(0, 2)) * invdet;
		minv._columns[2].x = +(m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1)) * invdet;
		minv._columns[2].y = -(m(0, 0) * m(2, 1) - m(2, 0) * m(0, 1)) * invdet;
		minv._columns[2].z = +(m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1)) * invdet;
		*this = minv;
	}

private:
	Vector3 _columns[3];
};

#endif // _MATRIX_H_
