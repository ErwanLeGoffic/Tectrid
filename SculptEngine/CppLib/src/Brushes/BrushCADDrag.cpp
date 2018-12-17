#include "BrushCADDrag.h"
#include "Mesh\OctreeVisitorCollectVertices.h"
#include "Mesh\OctreeVisitorRetessellateInRange.h"
#include "Math\Plane.h"
#include "Recorder\CommandRecorder.h"
#include "Mesh\ThicknessHandler.h"

//#define CADDRAG_RETESSELATE

void BrushCADDrag::UpdateStroke(Ray const& ray, float radius, float strengthRatio)
{
	CommandRecorder::GetInstance().Push(std::unique_ptr<Command>(new CommandUpdateStroke(_type, ray, radius, strengthRatio)));
	std::vector<unsigned int> const& triangles = _mesh.GetTriangles();
	std::vector<Vector3>& vertices = _mesh.GrabVertices();
	if(!_lastRay.IsValid())
	{	// First touching stroke update
		unsigned int intersectionTriangle = 0;
		if(_mesh.GetClosestIntersectionPointAndTriangle(ray, _initialDragPoint, &intersectionTriangle, &_initialDragNormal, true))
		{
			_dragDistance = (ray.GetOrigin() - _initialDragPoint).Length();
			SelectFace(intersectionTriangle, _faceTriangles);
			_faceVertices.clear();
			for(unsigned int triIdx : _faceTriangles)
			{
				_mesh.BanTriangleRetessellation(triIdx);
				_faceVertices.insert(triangles[triIdx * 3]);
				_faceVertices.insert(triangles[triIdx * 3 + 1]);
				_faceVertices.insert(triangles[triIdx * 3 + 2]);
			}
			_lastRay = ray;
		}		
	}
	else
	{	// Non-first stroke update
		// Compute displacement
		Vector3 lastDragPoint = _lastRay.GetOrigin() + (_lastRay.GetDirection() * _dragDistance);
		Vector3 curDragPoint = ray.GetOrigin() + (ray.GetDirection() * _dragDistance);
		Vector3 deltaMove = curDragPoint - lastDragPoint;
		// Project on the direction of the suface detected when we start the drag
		Vector3 deltaMoveProjected = _initialDragNormal * deltaMove.Dot(_initialDragNormal);
		// Move vertices
		for(unsigned int vtxIdx : _faceVertices)
		{
			vertices[vtxIdx] += deltaMoveProjected;
			_mesh.SetHasToRecomputeNormal(vtxIdx);
		}
#ifdef CADDRAG_RETESSELATE
		// Retesselate triangles touching the selected face
		ASSERT(_mesh.GrabRetessellator().IsReset());
		std::set<unsigned int> _touchingFaceTris;
		for(unsigned int vtxIdx : _faceVertices)
		{
			for(unsigned int triIdx : _mesh.GetTrianglesAroundVertex(vtxIdx))
			{
				if(_mesh.CanTriangleBeRetessellated(triIdx))
					_touchingFaceTris.insert(triIdx);
			}
		}
		if(!_touchingFaceTris.empty())
		{
			// Remove flat triangles: their smallest height get close to zero
			for(unsigned int triIdx : _touchingFaceTris)
				_mesh.GrabRetessellator().RemoveCrushedTriangle(triIdx, true);
			// Merge triangles with edges smaller than "d"
			for(unsigned int triIdx : _touchingFaceTris)
			{
				if(_mesh.IsTriangleToBeRemoved(triIdx))
					continue;
				unsigned int const* vtxsIdx = &(triangles[triIdx * 3]);
				unsigned int vtxId0 = vtxsIdx[0];
				unsigned int vtxId1 = vtxsIdx[1];
				unsigned int vtxId2 = vtxsIdx[2];
				Vector3 const& vtx0 = vertices[vtxId0];
				Vector3 const& vtx1 = vertices[vtxId1];
				Vector3 const& vtx2 = vertices[vtxId2];
				// Merge the smallest edge
				float squareLengthEdge1 = (vtx0 - vtx1).LengthSquared();
				float squareLengthEdge2 = (vtx1 - vtx2).LengthSquared();
				float squareLengthEdge3 = (vtx2 - vtx0).LengthSquared();
#ifdef MESH_CONSISTENCY_CHECK
				_mesh.CheckVertexIsCorrect(vtxId0);
				_mesh.CheckVertexIsCorrect(vtxId1);
				_mesh.CheckVertexIsCorrect(vtxId2);
#endif // MESH_CONSISTENCY_CHECK
				if((squareLengthEdge1 < squareLengthEdge2) && (squareLengthEdge1 < squareLengthEdge3))
					_mesh.GrabRetessellator().HandleEdgeMerge(triIdx, Retessellate::TRI_EDGE_1, false);
				else if(squareLengthEdge2 < squareLengthEdge3)
					_mesh.GrabRetessellator().HandleEdgeMerge(triIdx, Retessellate::TRI_EDGE_2, false);
				else
					_mesh.GrabRetessellator().HandleEdgeMerge(triIdx, Retessellate::TRI_EDGE_3, false);
			}
			// Subdivide triangles with edges taller than "d detail"
			for(unsigned int triIdx : _touchingFaceTris)
			{
				if(_mesh.IsTriangleToBeRemoved(triIdx))
					continue;
				unsigned int const* vtxsIdx = &(triangles[triIdx * 3]);
				unsigned int vtxId0 = vtxsIdx[0];
				unsigned int vtxId1 = vtxsIdx[1];
				unsigned int vtxId2 = vtxsIdx[2];
				Vector3 const& vtx0 = vertices[vtxId0];
				Vector3 const& vtx1 = vertices[vtxId1];
				Vector3 const& vtx2 = vertices[vtxId2];
				// will ask to subdivide the tallest edge, and merge the smallest
				float squareLengthEdge1 = (vtx0 - vtx1).LengthSquared();
				float squareLengthEdge2 = (vtx1 - vtx2).LengthSquared();
				float squareLengthEdge3 = (vtx2 - vtx0).LengthSquared();
				ASSERT(squareLengthEdge1 != 0.0f);
				ASSERT(squareLengthEdge2 != 0.0f);
				ASSERT(squareLengthEdge3 != 0.0f);
				if((squareLengthEdge1 >= squareLengthEdge2) && (squareLengthEdge1 >= squareLengthEdge3))
					_mesh.GrabRetessellator().HandleEdgeSubdiv(triIdx, Retessellate::TRI_EDGE_1, true);
				else if(squareLengthEdge2 >= squareLengthEdge3)
					_mesh.GrabRetessellator().HandleEdgeSubdiv(triIdx, Retessellate::TRI_EDGE_2, true);
				else
					_mesh.GrabRetessellator().HandleEdgeSubdiv(triIdx, Retessellate::TRI_EDGE_3, true);
			}
		}
#endif // CADDRAG_RETESSELATE
		//_mesh.GrabOctreeRoot().Traverse(retessellate);
		// Update octree, normals...
		_mesh.ReBalanceOctree(_mesh.GrabRetessellator().GetGenratedTris(), _mesh.GrabRetessellator().GetGenratedVtxs(), true);
		_mesh.RecomputeNormals(true, false);
		_mesh.RecomputeFragmentsBBox(false);
		_mesh.GrabRetessellator().Reset();
		_lastRay = ray;
	}
}

void BrushCADDrag::DoStroke(Vector3 const& /*curIntersectionPos*/, Vector3 const& /*curIntersectionNormal*/, float /*radius*/, float /*strengthRatio*/)
{
	ASSERT(false);	// Not used for the moment
}

void BrushCADDrag::SelectFace(unsigned int inputTriIdx, std::vector<unsigned int>& outputSelectedTris)
{
	static float smoothGroupBetweenFaceCosLimit = smoothGroupCosLimit;
	static float smoothGroupOverallFaceCosLimit = cosf(80.0f * float(M_PI) / 180.0f);
	std::vector<unsigned int> const& triangles = _mesh.GetTriangles();
	std::vector<Vector3> const& trisNormal = _mesh.GetTrisNormal();
	Vector3 const& initialRefNormal = trisNormal[inputTriIdx];
	std::vector<unsigned int> smoothGroup(_mesh.GetTrisNormal().size());
	for(unsigned int& elem : smoothGroup)
		elem = UNDEFINED_NEW_ID;
	unsigned int curSmoothGroup = 0;
	outputSelectedTris.clear();
	// Propagate smooth group inclusion until there is no surrounding triangles to treat (thus smooth group is complete)
	std::set<unsigned int> trisToTreat;
	trisToTreat.insert(inputTriIdx);
	outputSelectedTris.push_back(inputTriIdx);
	while(trisToTreat.empty() == false)
	{
		unsigned int triToTreat = *(trisToTreat.begin());
		Vector3 const& refNormal = trisNormal[triToTreat];
		trisToTreat.erase(trisToTreat.begin());
		unsigned int const* triToTreatVtxIdxs = &(triangles[triToTreat * 3]);
		// Prevent the smooth group to propagate on the other side of the hard edge
		bool processSurroundings = true;
		for(unsigned int i = 0; i < 3; ++i)
		{
			std::vector<unsigned int> const& trisAround = _mesh.GetTrianglesAroundVertex(triToTreatVtxIdxs[i]);
			for(unsigned int triAround : trisAround)
			{
				if((triAround != triToTreat) && (smoothGroup[triAround] == curSmoothGroup))
				{
					if(refNormal.Dot(trisNormal[triAround]) < smoothGroupBetweenFaceCosLimit)
					{
						if(trisNormal[triAround].LengthSquared() > 0.0f)	// Not a flat triangle
							processSurroundings = false;
					}
				}
			}
		}
		// Include all surrounding triangle if they fit in the smooth group
		if(processSurroundings)
		{
			for(unsigned int i = 0; i < 3; ++i)
			{
				std::vector<unsigned int> const& trisAround = _mesh.GetTrianglesAroundVertex(triToTreatVtxIdxs[i]);
				for(unsigned int triAround : trisAround)
				{
					if((triAround != triToTreat) && (smoothGroup[triAround] == UNDEFINED_NEW_ID))
					{
						bool includeInSmoothGroup = false;
						bool flatTriangle = false;
						if(initialRefNormal.Dot(trisNormal[triAround]) >= smoothGroupOverallFaceCosLimit)
						{	// Not too much out of facing regarding the triangle we used to select the face (to prevent to select a whole sphere for example)
							if(refNormal.Dot(trisNormal[triAround]) >= smoothGroupBetweenFaceCosLimit)
								includeInSmoothGroup = true; // The triangle touching each other are facing enough
						}
						else if(trisNormal[triAround].LengthSquared() == 0.0f)
						{
							includeInSmoothGroup = true; // Special case of flat triangles (that are generated for hard edges), we will include those in the selction
							flatTriangle = true;
						}
						if(includeInSmoothGroup)
						{	// Include in smooth group / face selction
							if(!flatTriangle)
								trisToTreat.insert(triAround);
							smoothGroup[triAround] = curSmoothGroup;
							outputSelectedTris.push_back(triAround);
						}
					}
				}
			}
		}
	}
}

void BrushCADDrag::EndStroke()
{
	Brush::EndStroke();
	_mesh.ClearAllTriangleRetessellationBan();
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(BrushCADDrag)
{
	class_<BrushCADDrag, base<Brush>>("BrushCADDrag")
		.constructor<Mesh&>();
}
#endif // __EMSCRIPTEN__