#ifndef _SCULPTENGINE_H_
#define _SCULPTENGINE_H_

#define PROFILE_INFO

#include <string>

const unsigned int UNDEFINED_NEW_ID = (unsigned int) ~0;

class SculptEngine
{
public:
	static void SetTriangleOrientationInverted(bool value)
	{
		_triangleOrientationInverted = value;
	}
	static bool IsTriangleOrientationInverted() { return _triangleOrientationInverted; }

	static void SetMirrorMode(bool value)
	{
		_mirrorMode = value;
	}
	static bool IsMirrorModeActivated() { return _mirrorMode; }

	static bool HasExpired();
	static std::string GetExpirationDate();

private:
	static bool _triangleOrientationInverted;
	static bool _mirrorMode;
};

#ifdef _DEBUG
	#include <stdio.h>
	
	bool TriggerDebugBreak();

	#define ASSERT(expression) (void) \
	( \
		(!!(expression)) || \
		(printf("!!! ASSERTION FAILED !!!: %s, %s, line: %d\n", #expression, __FILE__, (unsigned)(__LINE__)) == 0) || \
		TriggerDebugBreak() \
	)
#else
	#define ASSERT(expression) ((void)0)
#endif // !_DEBUG

#endif // _SCULPTENGINE_H_