#include "Ray.h"

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(Ray)
{
	class_<Ray>("Ray")
		.constructor<Vector3 const&, Vector3 const&, float>();
}
#endif // __EMSCRIPTEN__
