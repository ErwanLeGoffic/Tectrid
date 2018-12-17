#include "BrushInflate.h"
#include "Mesh\OctreeVisitorCollectVertices.h"
#include "Mesh\ThicknessHandler.h"

void BrushInflate::DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio)
{
	Brush::DoStroke(curIntersectionPos, curIntersectionNormal, radius, strengthRatio);
	// Collect vertices
	static float selectAngleCosLimit = cosf(100.0f * float(M_PI) / 180.0f);
	ThicknessHandler thicknessHandler(_mesh);
	OctreeVisitorCollectVertices collectVertices(_mesh, curIntersectionPos, radius, &curIntersectionNormal, selectAngleCosLimit, true, false, thicknessHandler);
	_mesh.GrabOctreeRoot().Traverse(collectVertices);
	// Apply distortion
	float radiusSquared = sqr(radius);
	float invRadius = 1.0f / radius;
	float effectRatio = lerp(0.01f, 0.17f, strengthRatio);
	std::vector<Vector3>& verticesFullmesh = _mesh.GrabVertices();
	std::vector<Vector3> const& normalsFullMesh = _mesh.GetNormals();
	std::vector<unsigned int> const& verticesIdx = collectVertices.GetVertices();
	for(unsigned int const& vtxIdx : verticesIdx)
	{
		Vector3& vertex = verticesFullmesh[vtxIdx];
		float distSquared = vertex.DistanceSquared(curIntersectionPos);
		if(distSquared < radiusSquared)
		{
			// Compute and apply position transformation
			float x = lerp(0.0f, 1.0f, sqrtf(distSquared) * invRadius);
			float xsquare = x * x;
			//float ratio = (cosf(x * float(M_PI)) / 2.0f) + 0.5f;
			float ratio = 3 * xsquare * xsquare - 4 * xsquare * x + 1;
			//if(Input.GetKey(KeyCode.LeftAlt))
			//	ratio = -ratio;
			float transformValue = ratio * radius * effectRatio;
			if(transformValue > 0.0f)
			{
				vertex += normalsFullMesh[vtxIdx] * transformValue;
				thicknessHandler.AddVertexToMovedList(vtxIdx);
				// Set vertex and surrounding triangles state to "dirty" regarding normal computing
				_mesh.SetHasToRecomputeNormal(vtxIdx);
			}
		}
		else
			ASSERT(false);
	}
	thicknessHandler.ReshapeRegardingThickness(ThicknessHandler::FUSION_MODE_MERGE);
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(BrushInflate)
{
	class_<BrushInflate, base<Brush>>("BrushInflate")
		.constructor<Mesh&>();
}
#endif // __EMSCRIPTEN__