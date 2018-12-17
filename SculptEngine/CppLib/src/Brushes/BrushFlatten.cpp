#include "BrushFlatten.h"
#include "Mesh\OctreeVisitorCollectVertices.h"
#include "Math\Plane.h"
#include "Mesh\ThicknessHandler.h"

void BrushFlatten::DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio)
{
	Brush::DoStroke(curIntersectionPos, curIntersectionNormal, radius, strengthRatio);
	// Collect vertices
	ThicknessHandler thicknessHandler(_mesh);
	OctreeVisitorCollectVertices collectVertices(_mesh, curIntersectionPos, radius, &curIntersectionNormal, 0.0f, true, true, thicknessHandler);
	_mesh.GrabOctreeRoot().Traverse(collectVertices);
	// Get average normal and build flatten plane
	Vector3 averageNormal = collectVertices.ComputeAverageNormal();
	Vector3 averagePosition = collectVertices.ComputeAveragePosition();
	Plane projectionPlane(averagePosition, averageNormal);
	// Apply distortion
	float radiusSquared = sqr(radius);
	float invRadius = 1.0f / radius;
	float effectRatio = lerp(0.1f, 1.0f, strengthRatio);
	std::vector<Vector3>& verticesFullmesh = _mesh.GrabVertices();
	auto ApplyDistorsion = [&](std::vector<unsigned int> const& verticesIdx)
	{
		for(unsigned int const& vtxIdx : verticesIdx)
		{
			Vector3& vertex = verticesFullmesh[vtxIdx];
			float distSquared = vertex.DistanceSquared(curIntersectionPos);
			if(distSquared < radiusSquared)
			{
				float distToPlane = projectionPlane.DistanceToVertex(vertex);
				// Compute and apply position transformation
				float x = lerp(0.0f, 1.0f, sqrtf(distSquared) * invRadius);
				//float ratio = 1.0f;
				//float ratio = (cosf(x * float(M_PI)) / 2.0f) + 0.5f;
				float ratio = 3 * pow(x, 4.0f) - 4 * pow(x, 3.0f) + 1;
				//if(Input.GetKey(KeyCode.LeftAlt))
				//	ratio = -ratio;
				//vertex += normalsFullMesh[vtxIdx] * ratio * -distToPlane;
				float transformValue = ratio * effectRatio * -distToPlane;
				if(transformValue != 0.0f)
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
	};
	ApplyDistorsion(collectVertices.GetVertices());
	ApplyDistorsion(collectVertices.GetRejectedVertices());	// Also flatten reject vertices (the one that are back oriented). This is to provide a consistent transformation: like you flatten a nose, and want the nostrils interior to be involved in the flatten.
	thicknessHandler.ReshapeRegardingThickness(ThicknessHandler::FUSION_MODE_PIERCE);
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(BrushFlatten)
{
	class_<BrushFlatten, base<Brush>>("BrushFlatten")
		.constructor<Mesh&>();
}
#endif // __EMSCRIPTEN__