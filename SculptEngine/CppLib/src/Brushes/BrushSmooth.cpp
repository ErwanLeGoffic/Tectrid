#include "BrushSmooth.h"
#include "Mesh\OctreeVisitorCollectVertices.h"
#include "Mesh\ThicknessHandler.h"

void BrushSmooth::DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio)
{
	Brush::DoStroke(curIntersectionPos, curIntersectionNormal, radius, strengthRatio);
	// Collect vertices
	static float selectAngleCosLimit = cosf(100.0f * float(M_PI) / 180.0f);
	ThicknessHandler thicknessHandler(_mesh);
	OctreeVisitorCollectVertices collectVertices(_mesh, curIntersectionPos, radius, nullptr, selectAngleCosLimit, true, false, thicknessHandler);
	_mesh.GrabOctreeRoot().Traverse(collectVertices);
	// Apply distortion
	std::vector<Vector3> verticesIn = _mesh.GetVertices();
	std::vector<Vector3>& verticesOut = _mesh.GrabVertices();
	std::vector<unsigned int> const& verticesIdx = collectVertices.GetVertices();
	for(unsigned int const& vtxIdx : verticesIdx)
	{
		Vector3 averageVertex;
		unsigned int nbVerticesInvolved = 0;
		if(1)
		{
			for(unsigned int triIdx : _mesh.GetTrianglesAroundVertex(vtxIdx))
			{
				unsigned int const* vtxIdxs = &(_mesh.GetTriangles()[triIdx * 3]);
				for(unsigned int i = 0; i < 3; ++i)
				{
					if(vtxIdxs[i] != vtxIdx)
					{
						averageVertex += verticesIn[vtxIdxs[i]];
						nbVerticesInvolved++;
					}
				}
			}
			verticesOut[vtxIdx] = averageVertex / float(nbVerticesInvolved);
			thicknessHandler.AddVertexToMovedList(vtxIdx);
		}
		else
		{
			float sqrLimit = sqr(radius / 10.0f);
			for(unsigned int const& vtxOtherIdx : verticesIdx)
			{
				if(verticesIn[vtxIdx].DistanceSquared(verticesIn[vtxOtherIdx]) < sqrLimit)
				{
					averageVertex += verticesIn[vtxOtherIdx];
					nbVerticesInvolved++;
				}
			}
			verticesOut[vtxIdx] = verticesIn[vtxIdx].Lerp(averageVertex / float(nbVerticesInvolved), 0.5f);
			thicknessHandler.AddVertexToMovedList(vtxIdx);
		}
		_mesh.SetHasToRecomputeNormal(vtxIdx);
	}
	thicknessHandler.ReshapeRegardingThickness(ThicknessHandler::FUSION_MODE_PIERCE);
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(BrushSmooth)
{
	class_<BrushSmooth, base<Brush>>("BrushSmooth")
		.constructor<Mesh&>();
}
#endif // __EMSCRIPTEN__