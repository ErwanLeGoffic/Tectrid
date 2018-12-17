#include "Matrix.h"

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(Matrix3)
{
	class_<Matrix3>("Matrix3")
		.constructor<Vector3 const&, Vector3 const&, Vector3 const&>();
}
#endif // __EMSCRIPTEN__