#include "BBox.h"

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(BBox)
{
	class_<BBox>("BBox")
		.constructor<>()
		.function("Min", &BBox::Min)
		.function("Max", &BBox::Max)
		.function("Center", &BBox::Center)
		.function("Size", &BBox::Size)
		.function("Extents", &BBox::Extents);
}
#endif // __EMSCRIPTEN__
