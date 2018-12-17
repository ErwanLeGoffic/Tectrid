#include "SubMesh.h"

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(SubMesh)
{
	class_<SubMesh>("SubMesh")
		.function("GetID", &SubMesh::GetID)
		.function("GetVersionNumber", &SubMesh::GetVersionNumber)
		.function("Triangles", &SubMesh::Triangles)
		.function("Vertices", &SubMesh::Vertices)
		.function("Normals", &SubMesh::Normals)
		.function("GetBBox", &SubMesh::GetBBox);
}
#endif // __EMSCRIPTEN__