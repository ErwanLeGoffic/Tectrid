#include "Vector.h"

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(Vector3)
{
	class_<Vector3>("Vector3")
		.constructor<const float, const float, const float>()
		.function("X", &Vector3::X)
		.function("Y", &Vector3::Y)
		.function("Z", &Vector3::Z)
		.function("Length", &Vector3::Length);
}
#endif // __EMSCRIPTEN__