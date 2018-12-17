#include "Math.h"
#include "Vector.h"
#include <float.h>

Vector3 ProjectPointOnLine(Vector3 const& point, Vector3 const& linePoint, Vector3 const& lineDir)
{
	Vector3 linePointToPoint = point - linePoint;
	float lineDirLengthSquared = lineDir.LengthSquared();
	if(lineDirLengthSquared > 0.0f)
		return linePoint + (lineDir * (lineDir.Dot(linePointToPoint) / lineDirLengthSquared));
	else
		return linePoint;
}

#ifdef _WIN32
#ifdef _DEBUG
class MathInit
{
public:
	MathInit()
	{
		// Activate floating point invalid values and operation break
		unsigned int state = _clearfp();
		state = _controlfp(0, 0);
		_controlfp(state & ~(/*_EM_INEXACT | */_EM_UNDERFLOW |
							 _EM_OVERFLOW | _EM_ZERODIVIDE |
							 _EM_INVALID | _EM_DENORMAL), _MCW_EM);
	}
};
static MathInit mathInit;
#endif // _DEBUG
#endif // _WIN32
