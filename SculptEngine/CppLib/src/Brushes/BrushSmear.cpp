#include "BrushSmear.h"
#include "Mesh\OctreeVisitorGetAverageInRange.h"

bool BrushSmear::Visitor::HasToVisit(OctreeCell& cell)
{
	return _rangeSphere.Intersects(cell.GetContentBBox());
}

void BrushSmear::Visitor::VisitEnter(OctreeCell& cell)
{
	if(!cell.GetTrianglesIdx().empty() || !cell.GetVerticesIdx().empty())
	{
		cell.AddStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH | CELL_STATE_HASTO_EXTRACT_OUTOFBOUNDS_GEOM);
		cell.AddStateFlagsUpToRoot(CELL_STATE_HASTO_RECOMPUTE_BBOX);
		std::vector<Vector3>& verticesFullmesh = _fullMesh.GrabVertices();
		std::vector<unsigned int> verticesIdx = cell.GetVerticesIdx();
		for(unsigned int const& vtxIdx : verticesIdx)
		{
			Vector3& vertex = verticesFullmesh[vtxIdx];
			float distSquared = vertex.DistanceSquared(_rangeSphere.GetCenter());
			if(distSquared < _radiusSquared)
			{
				// Compute and apply position transformation
				float x = lerp(0.0f, 1.0f, sqrtf(distSquared) * _invRadius);
				float xsquare = x * x;
				//float ratio = (cosf(x * float(M_PI)) / 2.0f) + 0.5f;
				float ratio = 3 * xsquare * xsquare - 4 * xsquare * x + 1;
				//if(Input.GetKey(KeyCode.LeftAlt))
				//	ratio = -ratio;
				float transformValue = ratio * _rangeSphere.GetRadius() * _smearRatio;
				if(transformValue > 0.0f)
				{
					vertex += _effectDisplacement * transformValue;
					// Set vertex and surrounding triangles state to "dirty" regarding normal computing
					_fullMesh.SetHasToRecomputeNormal(vtxIdx);
				}
			}
			else
				ASSERT(false);
		}
	}
}

void BrushSmear::DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio)
{
	Brush::DoStroke(curIntersectionPos, curIntersectionNormal, radius, strengthRatio);
	Vector3 effectDisplacement = _curRay.GetOrigin() - _lastRay.GetOrigin();
	if(effectDisplacement.LengthSquared() > 0.0f)
	{
		effectDisplacement.Normalize();
		VisitorGetAverageInRange getAverageVisitor(_mesh, curIntersectionPos, radius);
		_mesh.GrabOctreeRoot().Traverse(getAverageVisitor);
		Visitor smear(_mesh, getAverageVisitor.GetAveragePoint(), radius, strengthRatio, effectDisplacement);
		_mesh.GrabOctreeRoot().Traverse(smear);
	}
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(BrushSmear)
{
	class_<BrushSmear, base<Brush>>("BrushSmear")
		.constructor<Mesh&>();
}
#endif // __EMSCRIPTEN__