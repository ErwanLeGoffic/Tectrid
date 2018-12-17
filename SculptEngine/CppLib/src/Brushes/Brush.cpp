#include "Brush.h"
#include "Mesh\OctreeVisitorRetessellateInRange.h"
#include "Recorder\CommandRecorder.h"
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
#include "Mesh\OctreeVisitorHandlePendingRemovals.h"
#endif // CLEAN_PENDING_REMOVALS_IMMEDIATELY

void Brush::StartStroke()
{
	CommandRecorder::GetInstance().Push(std::unique_ptr<Command>(new CommandStartStroke(_type)));
#ifdef DEBUG_BRUSHES
	printf("***** START *****\n");
#endif // DEBUG_BRUSHES
	_strokeStarted = true;
	_lastRay.Invalidate();
	if(SculptEngine::IsMirrorModeActivated() && (_mirroredBrush != nullptr))
		_mirroredBrush->StartStroke();
}

void Brush::EndStroke()
{
	CommandRecorder::GetInstance().Push(std::unique_ptr<Command>(new CommandEndStroke(_type)));
#ifdef DEBUG_BRUSHES
	printf("----- END -----\n");
#endif // DEBUG_BRUSHES
	_strokeStarted = false;
	if(SculptEngine::IsMirrorModeActivated() && (_mirroredBrush != nullptr))
		_mirroredBrush->EndStroke();
	if(!SculptEngine::IsMirrorModeActivated() || (_mirroredBrush == nullptr))	// Don't take a snapshot (and other stuff) two time in mirror mode.
	{
#ifdef MESH_CONSISTENCY_CHECK
		_mesh.CheckMeshIsCorrect();
#endif // MESH_CONSISTENCY_CHECK
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
		_mesh.HandlePendingRemovals();
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
		_mesh.ReBalanceOctree(std::vector<unsigned int>(), std::vector<unsigned int>(), false);
		_mesh.TakeSnapShot();
	}
}

void Brush::UpdateStroke(Ray const& ray, float radius, float strengthRatio)
{
	CommandRecorder::GetInstance().Push(std::unique_ptr<Command>(new CommandUpdateStroke(_type, ray, radius, strengthRatio)));
	if(_lastRay.IsValid() && (radius > 0.0f))
	{
		Vector3 intersectionPos;
		Vector3 intersectionNormal;
		if(_mesh.GetClosestIntersectionPoint(ray, intersectionPos, &intersectionNormal, true))
		{
			_curRay = ray;
			// Project the two intersection points onto the line composed by the two rays (add their direction in case both rays start from the same point)
			// We need to do that instead of directly getting the delta of the intersection points because in case we are sculpting on a surface close to parallel from the sight vector, we will get big deltas in 3D even for time deltas in 2D as the intersection points will have a big delta depth.
			Vector3 deltaRaysOrigin = (ray.GetOrigin() + ray.GetDirection()) - (_lastRay.GetOrigin() + _lastRay.GetDirection());
			Vector3 deltaPos = ProjectPointOnLine(intersectionPos, _lastRay.GetOrigin(), deltaRaysOrigin) - ProjectPointOnLine(_lastIntersectionPos, _lastRay.GetOrigin(), deltaRaysOrigin);
			float delta = deltaPos.Length();
			// Compute time aliasing distance (distance between two actual strokes)
			float distThresholdForTimeAliasing = GetEffectRadiusPercentForTimeAliasing() * radius;
			if((distThresholdForTimeAliasing > 0.0f) && (delta >= distThresholdForTimeAliasing))
			{
				// Do some time aliasing on the sculpting points
				// Instead of having this : O      O        O
				// We will get this:        OOOOOOOOOOOOOOOOO
				float step = distThresholdForTimeAliasing / delta;
				Vector3 deltaRayPosStep = (ray.GetOrigin() - _lastRay.GetOrigin()) * step;
				Vector3 deltaRayDirStep = (ray.GetDirection() - _lastRay.GetDirection()) * step;
				float deltaRayLengthStep = (ray.GetLength() - _lastRay.GetLength()) * step;
				Vector3 curPos = _lastRay.GetOrigin() + deltaRayPosStep;
				Vector3 curDir = _lastRay.GetDirection() + deltaRayDirStep;
				float curLength = _lastRay.GetLength() + deltaRayLengthStep;				
				Vector3 curIntersectionPos = intersectionPos;
				for(float cursor = step; cursor <= 1.0f; cursor += step)	// Don't start with cursor at zero as we already apply brush at first UpdateStroke call
				{
					Vector3 curIntersectionNormal;
					_curRay = Ray(curPos, curDir.Normalized(), curLength);
					if(_mesh.GetClosestIntersectionPoint(_curRay, curIntersectionPos, &curIntersectionNormal, true))
					{
						DoStroke(curIntersectionPos, curIntersectionNormal, radius, strengthRatio);
						_mesh.RecomputeNormals(true);
						_mesh.RecomputeFragmentsBBox(false);
					}
#ifdef DEBUG_BRUSHES
					else
						printf("Missed 3\n");
#endif // DEBUG_BRUSHES
					curPos += deltaRayPosStep;
					curDir += deltaRayDirStep;	// Todo: implement a slerp
					curLength += deltaRayLengthStep;
				}
				/*#ifndef __EMSCRIPTEN__
				if((_lastIntersection - intersection).Length() > 0.0f)
				__debugbreak();
				#endif*/
				_lastIntersectionPos = curIntersectionPos;
				_lastRay = _curRay;
			}
			else if (delta > EPSILON)
			{	// No time aliasing
				if(distThresholdForTimeAliasing > 0.0f)
				{
					float dampedStrength = strengthRatio * lerp(0.0f, 1.0f, delta / distThresholdForTimeAliasing);	// Strength ratio at full only when reaching dist threshold
					DoStroke(intersectionPos, intersectionNormal, radius, dampedStrength);
				}
				else
					DoStroke(intersectionPos, intersectionNormal, radius, strengthRatio);
				_mesh.RecomputeNormals(true);
				_mesh.RecomputeFragmentsBBox(false);
				_lastIntersectionPos = intersectionPos;
				_lastRay = _curRay;
			}
		}
#ifdef DEBUG_BRUSHES
		else
			printf("Missed 2\n");
#endif // DEBUG_BRUSHES
	}
	else
	{
		Vector3 startIntersectionNormal;
		Vector3 startIntersectionPos;
		if(_mesh.GetClosestIntersectionPoint(ray, startIntersectionPos, &startIntersectionNormal, true))
		{
			_lastRay = ray;
			_lastIntersectionPos = startIntersectionPos;
			DoStroke(startIntersectionPos, startIntersectionNormal, radius, strengthRatio);
			_mesh.RecomputeNormals(true);
			_mesh.RecomputeFragmentsBBox(false);
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

void Brush::DoStroke(Vector3 const& curIntersectionPos, Vector3 const& /*curIntersectionNormal*/, float radius, float strengthRatio)
{
	ASSERT(_mesh.GrabRetessellator().IsReset());
	VisitorRetessellateInSphereRange retessellateInRange(_mesh, curIntersectionPos, radius * 1.1f, strengthRatio);	// Increase a bit retessellation zone to get a good retesselation around sculpting zone
	_mesh.GrabOctreeRoot().Traverse(retessellateInRange);
	// Insert generated vertices and triangle into the octree
	if((_mesh.GrabRetessellator().GetGenratedTris().size() != 0) || (_mesh.GrabRetessellator().GetGenratedVtxs().size() != 0))
	{
		_mesh.ReBalanceOctree(_mesh.GrabRetessellator().GetGenratedTris(), _mesh.GrabRetessellator().GetGenratedVtxs(), false);
		_mesh.RecomputeNormals(true);
		_mesh.RecomputeFragmentsBBox(false);
	}
	// Handle vertex and triangle removal
	if(_mesh.GrabRetessellator().WasSomethingMerged())
	{
#ifdef CLEAN_PENDING_REMOVALS_IMMEDIATELY
		_mesh.HandlePendingRemovals();
#else
		// Purge from octree vertices and triangles that were set to be removed and remap the vertices and triangles index of the one that will be moved
		OctreeVisitorHandlePendingRemovals handlePendingRemovals(_mesh, false);
		_mesh.GrabOctreeRoot().Traverse(handlePendingRemovals);
		// Now octree got rid of deleted vertices, now we will be able to recycle them
		_mesh.TransferPendingRemovesToRecycle();
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
	}
	_mesh.GrabRetessellator().Reset();
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(Brush)
{
	class_<Brush>("Brush")
		.function("StartStroke", &Brush::StartStroke)
		.function("UpdateStroke", &Brush::UpdateStroke)
		.function("EndStroke", &Brush::EndStroke);
}
#endif // __EMSCRIPTEN__