#include "BrushDrag.h"
#include "Mesh\OctreeVisitorCollectVertices.h"
#include "Math\Plane.h"
#include "Recorder\CommandRecorder.h"
#include "Mesh\ThicknessHandler.h"

void BrushDrag::UpdateStroke(Ray const& ray, float radius, float strengthRatio)
{
	CommandRecorder::GetInstance().Push(std::unique_ptr<Command>(new CommandUpdateStroke(_type, ray, radius, strengthRatio)));
	if(_lastRay.IsValid() && (radius > 0.0f))
	{	// Non-first stroke update
		_curRay = ray;
		_theoricalDestinationPoint = _curRay.GetOrigin() + _curRay.GetDirection() * _dragDistance;
		Vector3 dpl = _theoricalDestinationPoint - _refDraggedPoint;
		float dplLength = dpl.Length();
		const float minDplRadiusRatio = 0.5f;
		if(dplLength > (minDplRadiusRatio * radius))
		{
			// update drag motion vector
			dpl /= dplLength;	// Normalize
			_motionDir = lerp(_motionDir, dpl, (_nbDoStroke > 5) ? 0.1f : 0.01f);	// On the first step of the stroke we don't update the motion very fast: if we get too soon a change of direction we'll get a messed up result
			_motionDir.Normalize();
			// Do stroke
			DoStroke(_refDraggedPoint, _motionDir, radius, strengthRatio);
			_mesh.RecomputeNormals(true);
			_mesh.RecomputeFragmentsBBox(false);
			//_lastIntersectionPos = _refDraggedPoint;
			_lastRay = _curRay;
			_nbDoStroke++;
		}
	}
	else
	{	// First touching stroke update
		Vector3 startIntersectionNormal;
		Vector3 startIntersectionPos;
		if(_mesh.GetClosestIntersectionPoint(ray, startIntersectionPos, &startIntersectionNormal, true))
		{
			_refDraggedPoint = startIntersectionPos;
			_theoricalDestinationPoint = _refDraggedPoint;
			_dragDistance = (ray.GetOrigin() - _refDraggedPoint).Length();
			DoStroke(startIntersectionPos, startIntersectionNormal, radius, strengthRatio);
			_mesh.RecomputeNormals(true);
			_mesh.RecomputeFragmentsBBox(false);
			//_lastIntersectionPos = startIntersectionPos;
			_motionDir = startIntersectionNormal;
			_lastRay = ray;
			_nbDoStroke = 1;
		}
#ifdef DEBUG_BRUSHES
		else
			printf("Missed 1\n");
#endif // DEBUG_BRUSHES
	}
	if(SculptEngine::IsMirrorModeActivated() && (_mirroredBrush != nullptr))
	{
		Vector3 mirroredOrigin(-ray.GetOrigin().x, ray.GetOrigin().y, ray.GetOrigin().z);
		Vector3 mirroredDirection(-ray.GetDirection().x, ray.GetDirection().y, ray.GetDirection().z);
		Ray mirroredRay(mirroredOrigin, mirroredDirection, ray.GetLength());
		_mirroredBrush->UpdateStroke(mirroredRay, radius, strengthRatio);
	}
}

float BrushDrag::ComputeRadiusRatioFromAttractorDist(float radius)
{
	// Compute a ratio to make dragging thinner when attractor gets far from dragged reference point
	const float startToFallOffRadiusRatio = 2.0f;
	const float fallOffToZeroRadiusRatio = 10.0f;
	static float radiusRatio = 1.0f;
	float dplLength = (_theoricalDestinationPoint - _refDraggedPoint).Length();
	float dplLengthRatio = dplLength / radius;
	float clampedRadiusRatio = max(dplLengthRatio, startToFallOffRadiusRatio);
	clampedRadiusRatio = min(clampedRadiusRatio, fallOffToZeroRadiusRatio);
	float newRadiusRatio = (clampedRadiusRatio - startToFallOffRadiusRatio) / (fallOffToZeroRadiusRatio - startToFallOffRadiusRatio);
	newRadiusRatio = 1.0f - sinf(newRadiusRatio * float(M_PI_2));
	newRadiusRatio = clamp(newRadiusRatio, 0.01f, 1.0f);	// Don't want to get really nothing if too far
	radiusRatio += (newRadiusRatio - radiusRatio) * 0.1f;
	return radiusRatio;
}

void BrushDrag::DoStroke(Vector3 const& curIntersectionPos, Vector3 const& curIntersectionNormal, float radius, float strengthRatio)
{
	float adaptRadius = ComputeRadiusRatioFromAttractorDist(radius) * radius;	// Dragging thinner when attractor gets far
	_projectionSphereCenter = curIntersectionPos - (curIntersectionNormal * 0.5f * adaptRadius);
	Brush::DoStroke(_projectionSphereCenter, curIntersectionNormal, adaptRadius, strengthRatio);
	// Collect vertices
	ThicknessHandler thicknessHandler(_mesh);
	OctreeVisitorCollectVertices collectVertices(_mesh, _projectionSphereCenter, adaptRadius, nullptr/*&curIntersectionNormal*/, 0.0f, true, false, thicknessHandler);
	_mesh.GrabOctreeRoot().Traverse(collectVertices);
	// Apply distortion
	auto ApplyDistortion = [&](Vector3& vertex)->bool
	{
		// falloff at the border of influence
		float distFromIntersection = (vertex - curIntersectionPos).Length();
		float distRatio = distFromIntersection / adaptRadius;
		const float startToFallOff = 0.5f;
		float fallOff = lerp(1.0f, 0.0f, (clamp(distRatio, startToFallOff, 1.0f) - startToFallOff) * (1.0f / (1.0f - startToFallOff)));
		if(fallOff > 0.0f)
		{
			// Projection
			Vector3 dirSphereCenterToVertex = vertex - _projectionSphereCenter;
			float lengthSphereCenterToVertex = dirSphereCenterToVertex.Length();
			dirSphereCenterToVertex /= lengthSphereCenterToVertex;	// Normalize
			float dot = dirSphereCenterToVertex.Dot(curIntersectionNormal);
			if(dot > 0.0f)
			{	// Spherical
				float deltaTotalDist = adaptRadius - lengthSphereCenterToVertex;
				vertex += dirSphereCenterToVertex * (deltaTotalDist * fallOff * 0.1f);
				return true;
			}
			else
			{	// Cylindrical
				Vector3 pointOnAxis = _projectionSphereCenter + (curIntersectionNormal * dot * lengthSphereCenterToVertex);
				Vector3 axisToVertex = vertex - pointOnAxis;
				float axisToVertexLength = axisToVertex.Length();
				float deltaTotalDist = adaptRadius - axisToVertexLength;
				axisToVertex /= axisToVertexLength;	// Normalize
				vertex += axisToVertex * (deltaTotalDist * fallOff * 0.1f);
				return true;
			}
		}
		return false;
	};
	// Compute total displacement to do
	float distPercentToMove = lerp(0.001f, 0.05f, strengthRatio);
	Vector3 distFromAttractor = _theoricalDestinationPoint - _refDraggedPoint;
	float distToMove = distFromAttractor.Length() * distPercentToMove;
	// Compute displacement per step
	Vector3 refDraggedPointAfterDistortion = _refDraggedPoint;
	ApplyDistortion(refDraggedPointAfterDistortion);
	float maxDplPerDistortion = (refDraggedPointAfterDistortion - _refDraggedPoint).Length();
	// Apply distortion, step by step
	std::vector<Vector3>& verticesFullmesh = _mesh.GrabVertices();
	std::vector<unsigned int> const& verticesIdx = collectVertices.GetVertices();
	for(float curDisplacement = 0.0f; curDisplacement <= distToMove; curDisplacement += maxDplPerDistortion)
	{
		for(unsigned int const& vtxIdx : verticesIdx)
		{
			if(ApplyDistortion(verticesFullmesh[vtxIdx]))
			{
				_mesh.SetHasToRecomputeNormal(vtxIdx);
				thicknessHandler.AddVertexToMovedList(vtxIdx);
			}
		}
		//Vector3 refBeforeDistortion = _refDraggedPoint;
		ApplyDistortion(_refDraggedPoint);	// Transform our reference dragged point
		//_projectionSphereCenter += _refDraggedPoint - refBeforeDistortion;	// Note: I've commented this as moving projection point is the good thing, but it doesn't work if I don't re-tessellate each time I apply distortion step.
		thicknessHandler.ReshapeRegardingThickness(ThicknessHandler::FUSION_MODE_MERGE);
	}
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(BrushDrag)
{
	class_<BrushDrag, base<Brush>>("BrushDrag")
		.constructor<Mesh&>();
}
#endif // __EMSCRIPTEN__