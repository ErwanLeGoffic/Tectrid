#include "BrushDig.h"
#include "BrushFlatten.h"
#include "Mesh\OctreeVisitorCollectVertices.h"
#include "Math\Plane.h"
#include "Mesh\ThicknessHandler.h"

void BrushDig::DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio)
{
	Brush::DoStroke(curIntersectionPos, curIntersectionNormal, radius, strengthRatio);
	// Collect vertices
	ThicknessHandler thicknessHandler(_mesh);
	static float selectAngleCosLimit = cosf(100.0f * float(M_PI) / 180.0f);
#ifdef THICKNESS_HANDLER_WIP
	OctreeVisitorCollectVertices collectVertices(_mesh, curIntersectionPos, radius, &curIntersectionNormal, selectAngleCosLimit, true, false, thicknessHandler);
#else
	OctreeVisitorCollectVertices collectVertices(_mesh, curIntersectionPos, radius, nullptr, selectAngleCosLimit, true, false, thicknessHandler);
#endif // !THICKNESS_HANDLER_WIP
	_mesh.GrabOctreeRoot().Traverse(collectVertices);
	// Get average normal and build flatten plane
	Vector3 averageNormal = collectVertices.ComputeAverageNormal();
	Plane projectionPlane(curIntersectionPos + averageNormal * -0.5f * radius, averageNormal);
	// Apply distortion
	float radiusSquared = sqr(radius);
	float invRadius = 1.0f / radius;
	float effectRatio = lerp(0.01f, 0.39f, strengthRatio);
	std::vector<Vector3>& verticesFullmesh = _mesh.GrabVertices();
	std::vector<unsigned int> const& verticesIdx = collectVertices.GetVertices();
	for(unsigned int const& vtxIdx : verticesIdx)
	{
		Vector3& vertex = verticesFullmesh[vtxIdx];
		float distSquared = vertex.DistanceSquared(curIntersectionPos);
		if(distSquared < radiusSquared)
		{
			float distToPlane = projectionPlane.DistanceToVertex(vertex);
			// Compute and apply position transformation
			float x = lerp(0.0f, 1.0f, sqrtf(distSquared) * invRadius);
			float ratio = (cosf(x * float(M_PI)) / 2.0f) + 0.5f;
			float transformValue = ratio * effectRatio * -distToPlane;
			if(transformValue < 0.0f)
			{
				vertex += projectionPlane.GetNormal() * transformValue;
				thicknessHandler.AddVertexToMovedList(vtxIdx);
				// Set vertex and surrounding triangles state to "dirty" regarding normal computing
				_mesh.SetHasToRecomputeNormal(vtxIdx);
			}
		}
		else
			ASSERT(false);
	}
	thicknessHandler.ReshapeRegardingThickness(ThicknessHandler::FUSION_MODE_PIERCE);
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(BrushDig)
{
	class_<BrushDig, base<Brush>>("BrushDig")
		.constructor<Mesh&>();
}
#endif // __EMSCRIPTEN__