#ifndef _BBOX_H_
#define _BBOX_H_

#include "Math\Vector.h"
#include <float.h>

class BBox
{
public:
	BBox()
	{
		Reset();
	}

	BBox(Vector3 const& bound1, Vector3 const& bound2)
	{
		_min.x = min(bound1.x, bound2.x);
		_min.y = min(bound1.y, bound2.y);
		_min.z = min(bound1.z, bound2.z);
		_max.x = max(bound1.x, bound2.x);
		_max.y = max(bound1.y, bound2.y);
		_max.z = max(bound1.z, bound2.z);
	}

	BBox(BBox const& bbox)
	{
		_min = bbox._min;
		_max = bbox._max;
	}

	void Encapsulate(Vector3 const& point)
	{
		if(point.x < _min.x) _min.x = point.x; if(point.x > _max.x) _max.x = point.x;
		if(point.y < _min.y) _min.y = point.y; if(point.y > _max.y) _max.y = point.y;
		if(point.z < _min.z) _min.z = point.z; if(point.z > _max.z) _max.z = point.z;
	}

	void Encapsulate(BBox const& bbox)
	{
		Encapsulate(bbox._min);
		Encapsulate(bbox._max);
	}

	bool IsValid() const
	{
		return ((_min.x <= _max.x) && (_min.y <= _max.y) && (_min.z <= _max.z));
	}

	void Reset()
	{
		_min.x = FLT_MAX;
		_max.x = -FLT_MAX;
		_min.y = FLT_MAX;
		_max.y = -FLT_MAX;
		_min.z = FLT_MAX;
		_max.z = -FLT_MAX;
	}

	bool Contains(Vector3 const& point) const
	{
		if(point.x < _min.x) return false;
		else if(point.x >= _max.x) return false;
		else if(point.y < _min.y) return false;
		else if(point.y >= _max.y) return false;
		else if(point.z < _min.z) return false;
		else if(point.z >= _max.z) return false;
		return true;
	}

	void Scale(float ratio)
	{
		Vector3 const& boxCenter = Center();
		Vector3 boxExtentsScaled = Extents();
		boxExtentsScaled *= ratio;
		_min = boxCenter - boxExtentsScaled;
		_max = boxCenter + boxExtentsScaled;
	}

	void Translate(Vector3 const& translation)
	{
		_min += translation;
		_max += translation;
	}

	Vector3 const& Min() const { return _min; }
	Vector3 const& Max() const { return _max; }
	Vector3 Center() const { return (_max + _min) / 2.0f; }
	Vector3 Size() const { return (_max - _min); }
	Vector3 Extents() const { return (_max - _min) / 2.0f; }

	bool Intersects(BBox const& other) const
	{
		return (_min.x <= other._max.x) && (_max.x >= other._min.x) &&
			(_min.y <= other._max.y) && (_max.y >= other._min.y) &&
			(_min.z <= other._max.z) && (_max.z >= other._min.z);
	}

private:
	Vector3 _min;
	Vector3 _max;
};

#endif // _BBOX_H_