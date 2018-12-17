#ifndef _MATH_H_
#define _MATH_H_

#define THICKNESS_HANDLER_WIP
//#define RETESS_DEBUGGING
//#define LOW_DEF
#define MEDIUM_DEF

#define _USE_MATH_DEFINES
#include <math.h>

const float EPSILON = 0.00001f;
const double EPSILON_DBL = 0.00001;
const float EPSILON_SQR = EPSILON * EPSILON;

template <class T>
inline const T& min(const T& a, const T& b) { return (a < b) ? a : b; }

template <class T>
inline const T& max(const T& a, const T& b) { return (a > b) ? a : b; }

template <class T>
inline const T clamp(const T& value, const T& minLimit, const T& maxLimit) { return max(min(value, maxLimit), minLimit); }

template <class T>
inline const T lerp(const T& start, const T& end, float amount) { return start + ((end - start) * clamp(amount, 0.0f, 1.0f)); }

template <class T>
inline const T sqr(const T& value) { return value * value; }

inline float safe_acosf(float value) { return acosf(clamp(value, -1.0f, 1.0f)); }

template <class T>
inline void Swap(T& a, T& b) { T t = a; a = b; b = t; }

class Vector3;
Vector3 ProjectPointOnLine(Vector3 const& point, Vector3 const& linePoint, Vector3 const& lineDir);

#endif // _MATH_H_