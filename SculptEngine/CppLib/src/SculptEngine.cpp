#include "SculptEngine.h"
#include <time.h>
#include <stdio.h>

//#define EXPIRATIOn_DATE

bool SculptEngine::_triangleOrientationInverted = false;
bool SculptEngine::_mirrorMode = false;

#ifdef _DEBUG
static bool doBreak = true;
bool TriggerDebugBreak()
{
	if(doBreak)
		__debugbreak();
	return true;
}
#endif // _DEBUG

#ifdef EXPIRATIOn_DATE
static tm expirationDate = { 0/*sec*/, 0/*min*/, 0/*hour*/, 30/*month day*/, 5/*month*/, 118/*years since 1900*/, 6/*days since sunday*/, 180/*day number since jan 1st*/, 0/*daylight saving time flag*/ };
bool SculptEngine::HasExpired()
{
	time_t currentTime = time(NULL);   // get time now
	if(currentTime != -1)
	{
		/*struct tm *now = localtime(&currentTime);
		printf("now: %s\n", asctime(now));
		printf("limit: %s\n", asctime(&expirationDate));
		printf("time diff: %f\n", difftime(mktime(&expirationDate), currentTime));*/
		bool result = difftime(mktime(&expirationDate), currentTime) < 0;
		if(result)
			printf("Product has expired on %s. It won't function anymore, please update your version.\n", GetExpirationDate().c_str());
		return result;
	}
	return false;
}

std::string SculptEngine::GetExpirationDate()
{
	std::string month;
	switch(expirationDate.tm_mon)
	{
		case 0: month = "January"; break;
		case 1: month = "February"; break;
		case 2: month = "March"; break;
		case 3: month = "April"; break;
		case 4: month = "May"; break;
		case 5: month = "June"; break;
		case 6: month = "July"; break;
		case 7: month = "August"; break;
		case 8: month = "September"; break;
		case 9: month = "October"; break;
		case 10: month = "November"; break;
		case 11: month = "December"; break;
		default: month = "January"; break;
	}
	return std::string(month + std::string(" ") + std::to_string(expirationDate.tm_mday) + std::string(", ") + std::to_string(expirationDate.tm_year + 1900));
}
#else
bool SculptEngine::HasExpired()
{
	return false;
}

std::string SculptEngine::GetExpirationDate()
{
	return std::string("unlimited");
}
#endif // !EXPIRATIOn_DATE

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(SculptEngine)
{
	class_<SculptEngine>("SculptEngine")
		.class_function("SetTriangleOrientationInverted", &SculptEngine::SetTriangleOrientationInverted)
		.class_function("SetMirrorMode", &SculptEngine::SetMirrorMode)
		.class_function("IsMirrorModeActivated", &SculptEngine::IsMirrorModeActivated)
		.class_function("HasExpired", &SculptEngine::HasExpired)
		.class_function("GetExpirationDate", &SculptEngine::GetExpirationDate);
}
#endif // __EMSCRIPTEN__