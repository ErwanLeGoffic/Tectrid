#include "Retessellate.h"
#include "Mesh.h"
#include <set>
#ifdef RETESS_DEBUGGING
#include <string>
#endif // RETESS_DEBUGGING

Retessellate::Retessellate(Mesh& mesh): _mesh(mesh), _triangles(mesh.GrabTriangles()), _vertices(mesh.GrabVertices()), _trisNormal(mesh.GetTrisNormal()), _trisBSphere(mesh.GetTrisBSphere()), _somethingWasSubdivided(false), _somethingWasMerged(false)
{
	SetMaxEdgeLength(1.0f);
}

void Retessellate::SetMaxEdgeLength(float value)
{
	_dDetail = value;
	_dDetailSquared = _dDetail * _dDetail;
	_d = _dDetail * 0.4f;	// Minimum edge length
	_dSquared = _d * _d;
	_thickness = _dDetail * 1.1f;
	_thicknessSquared = sqr(_thickness);
}

bool Retessellate::HandleTriangleSubdiv(unsigned int triIdx, bool testLength)
{
	if(_mesh.IsTriangleToBeRemoved(triIdx))
		return false;	// Don't treat triangles that are to be removed
	std::vector<unsigned int> const& triangles = _mesh.GetTriangles();
	std::vector<Vector3>& vertices = _mesh.GrabVertices();
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
		return _mesh.GrabRetessellator().HandleEdgeSubdiv(triIdx, Retessellate::TRI_EDGE_1, testLength);
	else if(squareLengthEdge2 >= squareLengthEdge3)
		return _mesh.GrabRetessellator().HandleEdgeSubdiv(triIdx, Retessellate::TRI_EDGE_2, testLength);
	else
		return _mesh.GrabRetessellator().HandleEdgeSubdiv(triIdx, Retessellate::TRI_EDGE_3, testLength);
}

bool Retessellate::HandleEdgeSubdiv(unsigned int inputTriIdx, TRI_EDGE_NUMBER edgeNumber, bool testLength)
{
	unsigned int edgeVtx1 = 0, edgeVtx2 = 0;
	auto GetEdgeIndices = [&](TRI_EDGE_NUMBER edgeNumber)
	{
		switch(edgeNumber)
		{
		case TRI_EDGE_1:
			edgeVtx1 = _triangles[inputTriIdx * 3];
			edgeVtx2 = _triangles[inputTriIdx * 3 + 1];
			break;
		case TRI_EDGE_2:
			edgeVtx1 = _triangles[inputTriIdx * 3 + 1];
			edgeVtx2 = _triangles[inputTriIdx * 3 + 2];
			break;
		case TRI_EDGE_3:
			edgeVtx1 = _triangles[inputTriIdx * 3 + 2];
			edgeVtx2 = _triangles[inputTriIdx * 3];
			break;
		}
	};
	GetEdgeIndices(edgeNumber);
	Vector3 a = _vertices[edgeVtx1];
	Vector3 b = _vertices[edgeVtx2];
	Vector3 delta = b - a;
	float deltaLengthSquared = delta.LengthSquared();
	if((deltaLengthSquared > _dDetailSquared) || !testLength)
	{	// We have to subdivide
		bool isSpliitingTallestsEdges = true;	// We have to split the tallest edges of each triangle to provide the best homogeneous tesselation, thus we will subdivide surrounding triangles if needed
		std::vector<unsigned int> affectedTris;
		affectedTris.reserve(2);
#ifdef _DEBUG
		unsigned int nbLoops = 0;
#endif // _DEBUG
		do
		{
			ASSERT(nbLoops++ < 10);
			// Get affected triangles, this will be the triangles that are both around a and b
			if(!isSpliitingTallestsEdges)
				GetEdgeIndices(edgeNumber);	// Have to re-get vertices indices as isSpliitingTallestsEdges is false, so we have already loop once, and we have split stuff (and so maybe suppress triangles) at the end of the previous loop
			isSpliitingTallestsEdges = true;
			bool flatTriangleInResult = false;
			do
			{
				if(flatTriangleInResult)
				{
					GetEdgeIndices(edgeNumber);	// Have to re-get vertices indices as suppressing the flat triangle have changed indices
					flatTriangleInResult = false;
				}
				std::vector<unsigned int> const& triAroundA = _mesh.GetTrianglesAroundVertex(edgeVtx1);
				std::vector<unsigned int> const& triAroundB = _mesh.GetTrianglesAroundVertex(edgeVtx2);
				affectedTris.clear();
				for(unsigned int triIdxA : triAroundA)		// Todo : O(n), of course optimize this
				{
					for(unsigned int triIdxB : triAroundB)
					{
						if(triIdxA == triIdxB)
						{	// triangle common to vertex A and B
							ASSERT(_mesh.IsTriangleToBeRemoved(triIdxA) == false);
							if(_mesh.IsTriangleToBeRemoved(triIdxA))
								return false;	// We are working in the middle of triangles we already set to be deleted, don't continue. Todo: think about if continuing could be possible
							affectedTris.push_back(triIdxA);
							break;
						}
					}
				}
				ASSERT((affectedTris.size() == 2)
					|| ((affectedTris.size() == 1) && _mesh.IsVertexOnOpenEdge(edgeVtx1) && _mesh.IsVertexOnOpenEdge(edgeVtx2)));	// Should be always the case on a manifold mesh
				if(affectedTris.empty())
					return false;	// Definitively there's an issue here
				for(unsigned int triIdx : affectedTris)
					flatTriangleInResult |= _mesh.RemoveFlatTriangle(triIdx);
			} while(flatTriangleInResult);
			// Verify that we are splitting the tallest edge for each triangle
			for(unsigned int triIdx : affectedTris)
			{
				unsigned int vtxId0 = _triangles[triIdx * 3];
				unsigned int vtxId1 = _triangles[triIdx * 3 + 1];
				unsigned int vtxId2 = _triangles[triIdx * 3 + 2];
				Vector3 const& vtx0 = _vertices[vtxId0];
				Vector3 const& vtx1 = _vertices[vtxId1];
				Vector3 const& vtx2 = _vertices[vtxId2];
				float squareLengthEdge1 = (vtx0 - vtx1).LengthSquared();
				float squareLengthEdge2 = (vtx1 - vtx2).LengthSquared();
				float squareLengthEdge3 = (vtx2 - vtx0).LengthSquared();
				if((squareLengthEdge1 == 0.0f) || (squareLengthEdge2 == 0.0f) || (squareLengthEdge3 == 0.0f))
					return false;	// Splitting on a flat triangle, get out
				bool isSpliitingTallest = (deltaLengthSquared >= squareLengthEdge1) && (deltaLengthSquared >= squareLengthEdge2) && (deltaLengthSquared >= squareLengthEdge3);
				// If we're not, then split the triangle first
				if(!isSpliitingTallest)
				{
					isSpliitingTallestsEdges = false;
					if(!HandleTriangleSubdiv(triIdx, testLength))	// Todo: find another way than a recursive call
						return false;	// Can't subdivide triangles around, so abort splitting this
				}
			}
		} while(!isSpliitingTallestsEdges);
#ifdef MESH_CONSISTENCY_CHECK
		_mesh.CheckVertexIsCorrect(edgeVtx1);
		_mesh.CheckVertexIsCorrect(edgeVtx2);
#endif // MESH_CONSISTENCY_CHECK
		// Compute a point at the middle of a and b
		Vector3 halfDelta = delta * 0.5f;
		Vector3 newVertex(halfDelta + a);
		if(deltaLengthSquared < EPSILON_SQR)
		{	// When triangles are getting very tiny (edge measuring around 10-6), we get FPU roundings totally screwing mid-point calculation and ending in an endless loop as the triangle subdivision doesn't really occur (the mid point is most of the time the same than "b" or "a" because of rounding). Thus we detect this and return false to break the loop and cancel this triangle subdiv.
			if(halfDelta.x != 0.0f)
			{
				if((newVertex.x == a.x) || (newVertex.x == b.x))
					return false;
			}
			if(halfDelta.y != 0.0f)
			{
				if((newVertex.y == a.y) || (newVertex.y == b.y))
					return false;
			}
			if(halfDelta.z != 0.0f)
			{
				if((newVertex.z == a.z) || (newVertex.z == b.z))
					return false;
			}
		}

		for(unsigned int triIdx : affectedTris)
		{
			if(RemoveCrushedTriangle(triIdx, true))
				return false;	// We were trying to do subdivision on a triangle made of three coplanar vertices, this is now fixed, but just get out as things have changed a bit too much
		}

		unsigned int newVtxId = _mesh.AddVertex(newVertex);
		_newVerticesIDs.push_back(newVtxId);
		if(_mesh.IsVertexOnOpenEdge(edgeVtx1) && _mesh.IsVertexOnOpenEdge(edgeVtx2))
			_mesh.SetVertexOnOpenEdge(newVtxId);
#ifdef MESH_CONSISTENCY_CHECK
		for(unsigned int triIdx : affectedTris)
			_mesh.CheckTriangleIsCorrect(triIdx, true);
#endif // MESH_CONSISTENCY_CHECK
		// Subdivide triangles
		for(unsigned int triIdx : affectedTris)
			SubdivideTriangle(triIdx, edgeVtx1, edgeVtx2, newVtxId);
		// Remove flat triangle: If the three points of the triangle were coplanar and one of the three points was in the middle of the two others, we'll fall into that case
		for(unsigned int triIdx : affectedTris)
			_mesh.RemoveFlatTriangle(triIdx);	// Note: no need to remove the triangles generated by SubdivideTriangle, they will be deleted when deleting the original triangle next to them, see method "DeleteVertexAndReconnectTrianglesToAnotherOne()"
#ifdef MESH_CONSISTENCY_CHECK
		for(unsigned int triIdx : affectedTris)
			_mesh.CheckTriangleIsCorrect(triIdx, true);
#endif // MESH_CONSISTENCY_CHECK
		// Set recompute normal request after triangles are split as the request to recompute normal will spread on these triangles too
		_mesh.SetHasToRecomputeNormal(newVtxId);
		_somethingWasSubdivided = true;
		return true;
	}
	return false;
}

void Retessellate::SubdivideTriangle(unsigned int triIdx, unsigned int edgeVtx1, unsigned int edgeVtx2, unsigned int newVtx)
{
	// Find which triangle vertex corresponds to
	unsigned int* vtxsIdx = &(_triangles[triIdx * 3]);
	unsigned int posVtx1 = 0;
	unsigned int posVtx2 = 0;
	unsigned int posVtxOther = 0;
	for(unsigned int i = 0; i < 3; ++i)
	{
		if(vtxsIdx[i] == edgeVtx1) posVtx1 = i;
		else if(vtxsIdx[i] == edgeVtx2) posVtx2 = i;
		else posVtxOther = i;
	}
	ASSERT((posVtx1 != posVtx2) && (posVtxOther != posVtx1) && (posVtxOther != posVtx2));
	unsigned int newTriIdx = 0;
	unsigned int vtxOther = vtxsIdx[posVtxOther];	// Copy and not reference because below _triangles could be reallocated when pushing back a new triangle
	if(((posVtx1 < posVtx2) && ((posVtx1 != 0) || (posVtx2 != 2))) || ((posVtx1 == 2) && (posVtx2 == 0)))
	{	// Clockwise
		// Transform the triangle to be vtx1 - newVtx - vtxOther
		vtxsIdx[posVtx2] = newVtx;
		// Build a second triangle with newVtx - vtx2 - vtxOther
		newTriIdx = _mesh.AddTriangle(newVtx, edgeVtx2, vtxOther, _trisNormal[triIdx], false);
		_newTrianglesIDs.push_back(newTriIdx);
		// Update triangle around vertex DB
		_mesh.ChangeTriangleAroundVertex(edgeVtx2, triIdx, newTriIdx);
	}
	else
	{	// Counter clockwise
		// Transform the triangle to be vtx2 - newVtx - vtxOther
		vtxsIdx[posVtx1] = newVtx;
		// Build a second triangle with newVtx - vtx1 - vtxOther
		newTriIdx = _mesh.AddTriangle(newVtx, edgeVtx1, vtxOther, _trisNormal[triIdx], false);
		_newTrianglesIDs.push_back(newTriIdx);
		// Update triangle around vertex DB
		_mesh.ChangeTriangleAroundVertex(edgeVtx1, triIdx, newTriIdx);
	}
	// Update triangle around vertex DB
	_mesh.AddTriangleAroundVertex(newVtx, triIdx);
	_mesh.AddTriangleAroundVertex(newVtx, newTriIdx);
	_mesh.AddTriangleAroundVertex(vtxOther, newTriIdx);
}

void Retessellate::HandleEdgeMerge(unsigned int inputTriIdx, TRI_EDGE_NUMBER edgeNumber, bool doOnHardEdge)
{
	unsigned int edgeVtx1 = 0, edgeVtx2 = 0;
	auto GetEdgeIndices = [&](TRI_EDGE_NUMBER edgeNumber)
	{
		switch(edgeNumber)
		{
		case TRI_EDGE_1:
			edgeVtx1 = _triangles[inputTriIdx * 3];
			edgeVtx2 = _triangles[inputTriIdx * 3 + 1];
			break;
		case TRI_EDGE_2:
			edgeVtx1 = _triangles[inputTriIdx * 3 + 1];
			edgeVtx2 = _triangles[inputTriIdx * 3 + 2];
			break;
		case TRI_EDGE_3:
			edgeVtx1 = _triangles[inputTriIdx * 3 + 2];
			edgeVtx2 = _triangles[inputTriIdx * 3];
			break;
		}
	};
	GetEdgeIndices(edgeNumber);
	ASSERT(edgeVtx1 != edgeVtx2);
	if(edgeVtx1 == edgeVtx2)
		return;
	// Get edge square length
	Vector3 a = _vertices[edgeVtx1];
	Vector3 b = _vertices[edgeVtx2];
	Vector3 delta = b - a;
	float edgeSquareLength = delta.LengthSquared();
	// detect if we are partially on an open edge
	bool edgeVtx1OnOpenEdge = _mesh.IsVertexOnOpenEdge(edgeVtx1);
	bool edgeVtx2OnOpenEdge = _mesh.IsVertexOnOpenEdge(edgeVtx2);
	if(edgeVtx1OnOpenEdge != edgeVtx2OnOpenEdge)	// Don't treat this case for the moment (i.e one of the two vertices on an open edge)
	{
		if(edgeSquareLength != 0.0f)	// Will treat it anyway if the edge is zero-lengthed
			return;
	}
	if(edgeSquareLength < _dSquared)
	{	// We have to merge
		// Remove flat triangles
		auto RemoveFlatTriangle = [&](unsigned int& edgeVtx)
		{
			for(unsigned int i = 0; i < _mesh.GetTrianglesAroundVertex(edgeVtx).size();)
			{
				unsigned int triIdx = _mesh.GetTrianglesAroundVertex(edgeVtx)[i];
				bool wasFlatTriangleInResult = _mesh.RemoveFlatTriangle(triIdx);
				if(!wasFlatTriangleInResult)
					++i;
				else
				{
					unsigned int curEdgeVtxId = edgeVtx;
					GetEdgeIndices(edgeNumber);
					if(curEdgeVtxId != edgeVtx)
						i = 0;
				}
			}
		};
		RemoveFlatTriangle(edgeVtx1);
		RemoveFlatTriangle(edgeVtx2);
		// Remove degenerated triangles
		_mesh.DetectAndRemoveDegeneratedTrianglesAroundVertex(edgeVtx1, _newVerticesIDs);
		// Try to find degenerated edge: an edge that link more than two triangles. happens after collapsing an edge on thin parts
		_mesh.DetectAndTreatDegeneratedEdgeAroundVertex(edgeVtx1, _newVerticesIDs);
		// return if triangle was flat or degenerated and was deleted
		if(_mesh.IsTriangleToBeRemoved(inputTriIdx))
			return;
		// Get affected triangles, this will be a merge between triangles around "a" and "b"
		std::vector<unsigned int> const& triAroundA = _mesh.GetTrianglesAroundVertex(edgeVtx1);
		std::vector<unsigned int> const& triAroundB = _mesh.GetTrianglesAroundVertex(edgeVtx2);
		if(edgeSquareLength > _dSquared * 0.1f/*EPSILON_SQR*/)
		{
			if(triAroundA.size() > 8) return;
			if(triAroundB.size() > 8) return;	// Todo: see if we keep that limit
		}

		if((triAroundA.size() < 3) && !edgeVtx1OnOpenEdge)
		{	// We are treating degenerated triangles (they are two, and both share the same three vertices)
			_mesh.DetectAndRemoveDegeneratedTrianglesAroundVertex(edgeVtx1, _newVerticesIDs);
			return;
		}
		ASSERT((triAroundA.size() >= 3) || ((triAroundA.size() >= 1) && edgeVtx1OnOpenEdge));	// Should be always the case on a manifold mesh
		ASSERT((triAroundB.size() >= 3) || ((triAroundB.size() >= 1) && edgeVtx2OnOpenEdge));	// Should be always the case on a manifold mesh
#ifdef _DEBUG
		for(unsigned int triIdxA : triAroundA)
			ASSERT(_mesh.IsTriangleToBeRemoved(triIdxA) == false);
		for(unsigned int triIdxB : triAroundB)
			ASSERT(_mesh.IsTriangleToBeRemoved(triIdxB) == false);
#endif // _DEBUG
		// Collect triangle in common between A and B
		std::vector<unsigned int> affectedTriAroundA;
		std::vector<unsigned int> affectedTriAroundB;
		std::vector<unsigned int> triToRemove;
		affectedTriAroundA.reserve(triAroundA.size());
		affectedTriAroundB.reserve(triAroundB.size());
		affectedTriAroundA = triAroundA;
		for(unsigned int triIdxB : triAroundB)
		{
			bool alreadyExist = false;
			for(unsigned int triIdxA : triAroundA)		// Todo : O(n), of course optimize this
			{
				if(triIdxA == triIdxB)
				{
					alreadyExist = true;
					break;
				}
			}
			if(alreadyExist)
			{
				triToRemove.push_back(triIdxB);	// Remove the triangles that both share vertices "a" and "b"
				// Remove from affectedTri
				for(int i = 0; i < affectedTriAroundA.size(); ++i)
				{
					if(affectedTriAroundA[i] == triIdxB)
					{
						affectedTriAroundA[i] = affectedTriAroundA.back();
						affectedTriAroundA.pop_back();
					}
				}
			}
			else
				affectedTriAroundB.push_back(triIdxB);	// add triangle around "b" that are not around "a" (because those around "a" are already in affectedTri array)
		}
		ASSERT((triToRemove.size() == 2) || ((triToRemove.size() == 1) && edgeVtx1OnOpenEdge && edgeVtx2OnOpenEdge));	// Should be always the case on a manifold mesh.
		if((triToRemove.size() != 2) && ((triToRemove.size() != 1) || !edgeVtx1OnOpenEdge || !edgeVtx2OnOpenEdge))
			return;
		// Compute a point at the middle of "a" and "b"
		Vector3 middlePoint(delta * 0.5f + a);
		// Detect rejected edge merge cases
		//if(edgeSquareLength > _dSquared * 0.1f/*EPSILON_SQR*/)
		{
			// Detect that none of the affected triangles will be flipped after we suppressed the edge
			auto WillTriangleFlip = [&](unsigned int triIdx, unsigned int movedVtxIdx) -> bool
			{
				unsigned int* vtxsIdx = &(_triangles[triIdx * 3]);
				static std::vector<unsigned int> fixedVtxIdx;
				fixedVtxIdx.reserve(2);
				fixedVtxIdx.clear();
				for(unsigned int i = 0; i < 3; ++i)
				{
					unsigned int vtxIdx = vtxsIdx[i];
					if(vtxIdx != movedVtxIdx)
						fixedVtxIdx.push_back(vtxIdx);
				}
				// Compute triangle normal before modification
				Vector3 edge1Before = _vertices[fixedVtxIdx[1]] - _vertices[movedVtxIdx];
				Vector3 edge2Before = _vertices[fixedVtxIdx[0]] - _vertices[movedVtxIdx];
				Vector3 normalBefore = edge1Before.Cross(edge2Before);
				// Compute triangle normal after modification
				Vector3 edge1After = _vertices[fixedVtxIdx[1]] - middlePoint;
				Vector3 edge2After = _vertices[fixedVtxIdx[0]] - middlePoint;
				Vector3 normalAfter = edge1After.Cross(edge2After);
				if(normalBefore.Dot(normalAfter) <= 0.0f)
					return true;
				return false;
			};
			// Test triangles won't flip
			for(unsigned int triIdxA : triAroundA)
			{
				if(WillTriangleFlip(triIdxA, edgeVtx1))
					return;
			}
			for(unsigned int triIdxB : triAroundB)
			{
				if(WillTriangleFlip(triIdxB, edgeVtx2))
					return;
			}
			// Don't merge on hard edge
			if(!doOnHardEdge)
			{
				static float cosLimit = cosf(50.0f * float(M_PI) / 180.0f);
				unsigned int triToRemove0Idx = triToRemove[0];
				if(triToRemove.size() >= 2)
				{
					unsigned int triToRemove1Idx = triToRemove[1];
					float cosAngle = _trisNormal[triToRemove0Idx].Dot(_trisNormal[triToRemove1Idx]);
					if(cosAngle < cosLimit)
						return;
				}
				for(unsigned int triIdxA : triAroundA)
				{
					float cosAngle = _trisNormal[triToRemove0Idx].Dot(_trisNormal[triIdxA]);
					if(cosAngle < cosLimit)
						return;
				}
				for(unsigned int triIdxB : triAroundB)
				{
					float cosAngle = _trisNormal[triToRemove0Idx].Dot(_trisNormal[triIdxB]);
					if(cosAngle < cosLimit)
						return;
				}
			}
		}
#ifdef _DEBUG
/*		std::vector<unsigned int> vtxIdxToDebugAroundBefore;
		vtxIdxToDebugAroundBefore.push_back(edgeVtx1);
		vtxIdxToDebugAroundBefore.push_back(edgeVtx2);
		_mesh.SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "before.obj");*/
#endif // _DEBUG
		// Ok, do merging
		_somethingWasMerged = true;
		// Replace "a" vertex with the middle coordinate
		_vertices[edgeVtx1] = delta * 0.5f + a;
		// For each triangle that was around "b", relink to "a" vertex
		for(unsigned int triIdxB : triAroundB)
		{
			if((triIdxB != triToRemove[0]) && ((triToRemove.size() < 2) || (triIdxB != triToRemove[1])))
			{
				unsigned int* vtxsIdx = &(_triangles[triIdxB * 3]);
				unsigned int i = 0;
				for(; i < 3; ++i)
				{
					if(vtxsIdx[i] == edgeVtx2)
					{
						vtxsIdx[i] = edgeVtx1;
						_mesh.AddTriangleAroundVertex(edgeVtx1, triIdxB);
						break;
					}
				}
				ASSERT(i < 3);	// We should have find something
			}
		}
		_mesh.ClearTriangleAroundVertex(edgeVtx2);
		// Register "b" vertex for removal
		_mesh.SetVertexToBeRemoved(edgeVtx2);
		// Register triToRemove for removal
		_mesh.DeleteTriangle(triToRemove[0]);
		if(triToRemove.size() >= 2)
			_mesh.DeleteTriangle(triToRemove[1]);
		// Remove degenerated triangles
		_mesh.DetectAndRemoveDegeneratedTrianglesAroundVertex(edgeVtx1, _newVerticesIDs);
		// Try to find degenerated edge: an edge that link more than two triangles. happens after collapsing an edge on thin parts
		_mesh.DetectAndTreatDegeneratedEdgeAroundVertex(edgeVtx1, _newVerticesIDs);
#ifdef _DEBUG
/*		std::vector<unsigned int> vtxIdxToDebugAroundAfter;
		vtxIdxToDebugAroundAfter.push_back(edgeVtx1);
		_mesh.SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundAfter, "after.obj");*/
#endif // _DEBUG
		// Set recompute normal request
		if(_mesh.IsVertexToBeRemoved(edgeVtx1) == false)	// The vertex could be tagged to be removed: This happens if all resulting edges and triangles were degenerated.
			_mesh.SetHasToRecomputeNormal(edgeVtx1);
	}
}

bool Retessellate::RemoveCrushedTriangle(unsigned int triIdx, bool forceCompletelyFlatRemoval)
{	// Remove triangles that are crushed: like one of the vertex get close to its opposite edge
	static const float squaredHeightRatioThreshold = sqr(0.5f); // Ratio regarding _dSquared distance: it's applied on the measurement with projection of the point opposite to tallest edge
	unsigned int* vtxsIdx = &(_triangles[triIdx * 3]);
	unsigned int& vtxId0 = vtxsIdx[0];	// Note: take reference on indices as the upcoming removeFlatTriangles() call could change those
	unsigned int& vtxId1 = vtxsIdx[1];
	unsigned int& vtxId2 = vtxsIdx[2];
	Vector3 const& vtx0 = _vertices[vtxId0];
	Vector3 const& vtx1 = _vertices[vtxId1];
	Vector3 const& vtx2 = _vertices[vtxId2];
	Vector3 const edge1(vtx1 - vtx0);
	Vector3 const edge2(vtx2 - vtx1);
	Vector3 const edge3(vtx0 - vtx2);
	// will find the tallest edge
	float squaredLengthEdge1 = edge1.LengthSquared();
	float squaredLengthEdge2 = edge2.LengthSquared();
	float squaredLengthEdge3 = edge3.LengthSquared();
	if(!forceCompletelyFlatRemoval && ((squaredLengthEdge1 < _dSquared) || (squaredLengthEdge2 < _dSquared) || (squaredLengthEdge3 < _dSquared)))
	{	// Don't need to check if that triangle is crushed or not because one of its edge is already small enough to me suppressed by edge merger
		if((squaredLengthEdge1 == 0.0f) || (squaredLengthEdge2 == 0.0f) || (squaredLengthEdge3 == 0.0f))
			return _mesh.RemoveFlatTriangle(triIdx);
		return false;
	}
	Vector3 const* tallestEdge;
	Vector3 const* tallestEdgeStart;
	Vector3 const* vertexOpositeToTallestEdge;
	float squaredLengthTallestEdge;
	unsigned int *tallEdgeVtxId1, *tallEdgeVtxId2, *otherVtxId;	// Note: take pointer on indices as the upcoming removeFlatTriangles() call could change those
	TRI_EDGE_NUMBER tallestEdgeNumber;
	if((squaredLengthEdge1 >= squaredLengthEdge2) && (squaredLengthEdge1 >= squaredLengthEdge3))
	{
		tallestEdge = &edge1;
		tallestEdgeStart = &vtx0;
		vertexOpositeToTallestEdge = &vtx2;
		squaredLengthTallestEdge = squaredLengthEdge1;
		tallEdgeVtxId1 = &vtxId0;
		tallEdgeVtxId2 = &vtxId1;
		otherVtxId = &vtxId2;
		tallestEdgeNumber = TRI_EDGE_1;
	}
	else if(squaredLengthEdge2 >= squaredLengthEdge3)
	{
		tallestEdge = &edge2;
		tallestEdgeStart = &vtx1;
		vertexOpositeToTallestEdge = &vtx0;
		squaredLengthTallestEdge = squaredLengthEdge2;
		tallEdgeVtxId1 = &vtxId1;
		tallEdgeVtxId2 = &vtxId2;
		otherVtxId = &vtxId0;
		tallestEdgeNumber = TRI_EDGE_2;
	}
	else
	{
		tallestEdge = &edge3;
		tallestEdgeStart = &vtx2;
		vertexOpositeToTallestEdge = &vtx1;
		squaredLengthTallestEdge = squaredLengthEdge3;
		tallEdgeVtxId1 = &vtxId2;
		tallEdgeVtxId2 = &vtxId0;
		otherVtxId = &vtxId1;
		tallestEdgeNumber = TRI_EDGE_3;
	}
	if(_mesh.IsVertexOnOpenEdge(*tallEdgeVtxId1) && _mesh.IsVertexOnOpenEdge(*tallEdgeVtxId2))
		return false;	// We don't do anything concerning triangles on open edge
	// Project on tallest edge its opposite vertex
	float lengthTallestEdge = sqrtf(squaredLengthTallestEdge);
	Vector3 const tallestEdgeDir = *tallestEdge / lengthTallestEdge;
	float dot = tallestEdgeDir.Dot(*vertexOpositeToTallestEdge - *tallestEdgeStart);
	ASSERT((dot > 0.0f) && (dot < lengthTallestEdge));
	Vector3 const projectedPoint = *tallestEdgeStart + tallestEdgeDir * dot;
	// Get distance from opposite vertex to its projection on tallest edge, this is what we take in account to determine if the triangle is crushed or not
	float distSquaredOpositeToProjected = (*vertexOpositeToTallestEdge - projectedPoint).LengthSquared();
	if(distSquaredOpositeToProjected < (_dSquared * squaredHeightRatioThreshold))
	{	// Triangle crushed
		if(distSquaredOpositeToProjected >= EPSILON)
		{
			if(forceCompletelyFlatRemoval == false)	// Do we only care about completely flat triangles?
				return HandleEdgeSubdiv(triIdx, tallestEdgeNumber, false);	// Don't have to remove the triangles, short edge removal will do the job
		}
		else
		{
			// Get the other triangle sharing the tallest edge
			std::vector<unsigned int> const* triAroundA = &(_mesh.GetTrianglesAroundVertex(*tallEdgeVtxId1));
			std::vector<unsigned int> const* triAroundB = &(_mesh.GetTrianglesAroundVertex(*tallEdgeVtxId2));	// Note: take pointers as the upcoming removeFlatTriangles() call could change those
			unsigned int otherTriangleID = UNDEFINED_NEW_ID;
			bool flatTriangleDetected = false;
			do
			{
				for(unsigned int triIdxA : *triAroundA)		// Todo : O(n), of course optimize this
				{
					for(unsigned int triIdxB : *triAroundB)
					{
						if(triIdxA == triIdxB)
						{	// triangle common to vertex A and B
							if(triIdx != triIdxA)
								otherTriangleID = triIdxA;
							break;
						}
					}
					if(otherTriangleID != UNDEFINED_NEW_ID)
						break;
				}
				if(otherTriangleID != UNDEFINED_NEW_ID)
				{	// Flat triangle removal
					flatTriangleDetected = _mesh.RemoveFlatTriangle(otherTriangleID);
					if(flatTriangleDetected)
					{
						triAroundA = &(_mesh.GetTrianglesAroundVertex(*tallEdgeVtxId1));
						triAroundB = &(_mesh.GetTrianglesAroundVertex(*tallEdgeVtxId2));
						otherTriangleID = UNDEFINED_NEW_ID;
					}
				}
			} while(flatTriangleDetected);
			ASSERT(otherTriangleID != UNDEFINED_NEW_ID);
			if(otherTriangleID != UNDEFINED_NEW_ID)
			{
				unsigned int otherTriangleOppositeVertexID = _triangles[otherTriangleID * 3];
				if((otherTriangleOppositeVertexID == *tallEdgeVtxId1) || (otherTriangleOppositeVertexID == *tallEdgeVtxId2))
					otherTriangleOppositeVertexID = _triangles[otherTriangleID * 3 + 1];
				if((otherTriangleOppositeVertexID == *tallEdgeVtxId1) || (otherTriangleOppositeVertexID == *tallEdgeVtxId2))
					otherTriangleOppositeVertexID = _triangles[otherTriangleID * 3 + 2];
				// Verify the triangles won't flip
				unsigned int triModified[3];
				for(int i = 0; i < 3; ++i)
				{
					triModified[i] = _triangles[triIdx * 3 + i];
					if(triModified[i] == *tallEdgeVtxId2)	// Replace tallEdgeVtxId2 with otherTriangleOppositeVertexID on first triangle
						triModified[i] = otherTriangleOppositeVertexID;
				}
				Vector3 newNormal;
				ComputeTriNormal(triModified, newNormal);
				if(newNormal.Dot(_trisNormal[triIdx]) < 0.0f)
					return false;	// triangle would flip, get out!
				for(int i = 0; i < 3; ++i)
				{
					triModified[i] = _triangles[otherTriangleID * 3 + i];
					if(triModified[i] == *tallEdgeVtxId1)	// Replace tallEdgeVtxId1 with otherVtxId on second triangle
						triModified[i] = *otherVtxId;
				}
				ComputeTriNormal(triModified, newNormal);
				if(newNormal.Dot(_trisNormal[otherTriangleID]) < 0.0f)
					return false;	// triangle would flip, get out!
				// Triangle edge switch
#ifdef MESH_CONSISTENCY_CHECK
				_mesh.CheckVertexIsCorrect(*tallEdgeVtxId1);
				_mesh.CheckVertexIsCorrect(*tallEdgeVtxId2);
				_mesh.CheckTriangleIsCorrect(triIdx, true);
				_mesh.CheckTriangleIsCorrect(otherTriangleID, true);
#endif // MESH_CONSISTENCY_CHECK
#ifdef _DEBUG
				/*std::vector<unsigned int> vtxIdxToDebugAroundBefore;
				vtxIdxToDebugAroundBefore.push_back(tallEdgeVtxId1);
				vtxIdxToDebugAroundBefore.push_back(tallEdgeVtxId2);
				_mesh.SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "before.obj");*/
#endif // _DEBUG
				// Replace tallEdgeVtxId2 with otherTriangleOppositeVertexID on first triangle
				for(int i = 0; i < 3; ++i)
				{
					if(_triangles[triIdx * 3 + i] == *tallEdgeVtxId2)
					{
						_mesh.RemoveTriangleAroundVertex(*tallEdgeVtxId2, triIdx);
						_mesh.AddTriangleAroundVertex(otherTriangleOppositeVertexID, triIdx);
						_triangles[triIdx * 3 + i] = otherTriangleOppositeVertexID;
						break;
					}
				}
				// Replace tallEdgeVtxId1 with otherVtxId on second triangle
				for(int i = 0; i < 3; ++i)
				{
					if(_triangles[otherTriangleID * 3 + i] == *tallEdgeVtxId1)
					{
						_mesh.RemoveTriangleAroundVertex(*tallEdgeVtxId1, otherTriangleID);
						_mesh.AddTriangleAroundVertex(*otherVtxId, otherTriangleID);
						_triangles[otherTriangleID * 3 + i] = *otherVtxId;
						break;
					}
				}
#ifdef _DEBUG
				//_mesh.SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "after.obj");
#endif // _DEBUG
				_mesh.SetHasToRecomputeNormal(*tallEdgeVtxId1);
				_mesh.SetHasToRecomputeNormal(*tallEdgeVtxId2);
#ifdef MESH_CONSISTENCY_CHECK
				_mesh.CheckTriangleIsCorrect(triIdx, true);
				_mesh.CheckTriangleIsCorrect(otherTriangleID, true);
#endif // MESH_CONSISTENCY_CHECK
				return true;
			}
		}
	}
	return false;
}

void Retessellate::HandlePendingRemovals()
{
	// Update _newTrianglesIDs data
	for(unsigned int i = 0; i < _newTrianglesIDs.size();)
	{
		unsigned int& triIdx = _newTrianglesIDs[i];
		if(_mesh.IsTriangleToBeRemoved(triIdx))
		{	// Remove element
			triIdx = _newTrianglesIDs.back();
			_newTrianglesIDs.pop_back();
		}
		else
		{
			if(_mesh.TriHasToMove(triIdx))
				triIdx = _mesh.GetNewTriIdx(triIdx);	// Remap element
			++i;
		}
	}
	// Update _newVerticesIDs data
	for(unsigned int i = 0; i < _newVerticesIDs.size();)
	{
		unsigned int& vtxIdx = _newVerticesIDs[i];
		if(_mesh.IsVertexToBeRemoved(vtxIdx))
		{	// Remove element
			vtxIdx = _newVerticesIDs.back();
			_newVerticesIDs.pop_back();
		}
		else
		{
			if(_mesh.VtxHasToMove(vtxIdx))
				vtxIdx = _mesh.GetNewVtxIdx(vtxIdx);	// Remap element
			++i;
		}
	}
}

void Retessellate::ComputeTriNormal(unsigned int indices[3], Vector3& resultingNormal)
{
	Vector3& vtx1 = _vertices[indices[0]];
	Vector3& vtx2 = _vertices[indices[1]];
	Vector3& vtx3 = _vertices[indices[2]];

	Vector3 p1p2 = vtx2 - vtx1;
	Vector3 p1p3 = vtx3 - vtx1;

	resultingNormal = p1p2.Cross(p1p3);
	float normalLength = resultingNormal.Length();
	if(normalLength != 0.0f)
	{
		resultingNormal /= normalLength;
		if(SculptEngine::IsTriangleOrientationInverted())
			resultingNormal.Negate();
	}
}

void Retessellate::StichLoops(LoopElement const* rootLoopA, unsigned int vtxToEliminateOnAIdx, LoopElement const* rootLoopB, unsigned int vtxToEliminateOnBIdx, unsigned int loopALength, unsigned int loopBLength, bool loopAreSameDir)
{
	auto StitchVtx = [&](unsigned int curLoopVtx, unsigned int nextLoopVtx, unsigned int vtxToEliminateIdx, unsigned int vtxToStichToIdx)
	{
		// Find triangle curLoopVtx - nextLoopVtx - vtxToEliminateIdx
		std::vector<unsigned int> const& triAround = _mesh.GetTrianglesAroundVertex(vtxToEliminateIdx);
		for(unsigned int triIdx : triAround)
		{
			unsigned int* triVtxIdxs = &(_triangles[triIdx * 3]);
			unsigned int posCur = UNDEFINED_NEW_ID;
			unsigned int posNext = UNDEFINED_NEW_ID;
			unsigned int posVtxToEliminateIdx = UNDEFINED_NEW_ID;
			for(unsigned int triVtxIdx = 0; triVtxIdx < 3; ++triVtxIdx)
			{
				if(triVtxIdxs[triVtxIdx] == curLoopVtx)
					posCur = triVtxIdx;
				else if(triVtxIdxs[triVtxIdx] == nextLoopVtx)
					posNext = triVtxIdx;
				else if(triVtxIdxs[triVtxIdx] == vtxToEliminateIdx)
					posVtxToEliminateIdx = triVtxIdx;
			}
			if((posCur != UNDEFINED_NEW_ID) && (posNext != UNDEFINED_NEW_ID) && (posVtxToEliminateIdx != UNDEFINED_NEW_ID))
			{	// We found the triangle, change vtxToEliminateIdx with curBVtx
				if((vtxToStichToIdx == curLoopVtx) || (vtxToStichToIdx == nextLoopVtx))
				{	// We are stiching two loops that share vertices. When we fall on such triangles (loop connectivity zone), they are lines, supress thos triangles
					_mesh.DeleteTriangle(triIdx);
				}
				else
				{
					if(_mesh.GetTrianglesAroundVertex(vtxToEliminateIdx).size() == 1)
						_mesh.SetVertexToBeRemoved(vtxToEliminateIdx);
					_mesh.RemoveTriangleAroundVertex(vtxToEliminateIdx, triIdx);
					_mesh.AddTriangleAroundVertex(vtxToStichToIdx, triIdx);
					triVtxIdxs[posVtxToEliminateIdx] = vtxToStichToIdx;
				}
				break;
			}
		}
	};
	{	// Find in the closest point pair between loop A and B
		LoopElement const* curAVtx = rootLoopA;
		float curClosestDistSquared = FLT_MAX;
		LoopElement const* curClosestAVtx = rootLoopA;
		LoopElement const* curClosestBVtx = rootLoopB;
		do
		{
			LoopElement const* curBVtx = rootLoopB;
			Vector3 const& curAVtxPos = _vertices[curAVtx->GetVtxId()];
			do
			{
				float distSquared = (_vertices[curBVtx->GetVtxId()] - curAVtxPos).LengthSquared();
				if(distSquared < curClosestDistSquared)
				{
					curClosestAVtx = curAVtx;
					curClosestBVtx = curBVtx;
					curClosestDistSquared = distSquared;
				}
				curBVtx = curBVtx->GetNext();
			} while(curBVtx != rootLoopB);
			curAVtx = curAVtx->GetNext();
		} while(curAVtx != rootLoopA);
		rootLoopA = curClosestAVtx;
		rootLoopB = curClosestBVtx;
	}
	if(loopBLength > loopALength)
	{	// Bresenham needs that the reference axis (here A loop) is the longest one
		StichLoops(rootLoopB, vtxToEliminateOnBIdx, rootLoopA, vtxToEliminateOnAIdx, loopBLength, loopALength, loopAreSameDir);
		return;
	}
#ifdef RETESS_DEBUGGING
	static int count = 0;
	count++;

	if(0)//count == 379)
	{
#ifdef _DEBUG
		consistencyCheckOn = true;
		// Check vertices
		LoopElement const* curVtx = rootLoopA;
		do
		{
			_mesh.CheckVertexIsCorrect(curVtx->GetVtxId());
			curVtx = curVtx->GetNext();
		} while(curVtx != rootLoopA);
		curVtx = rootLoopB;
		do
		{
			_mesh.CheckVertexIsCorrect(curVtx->GetVtxId());
			curVtx = curVtx->GetNext();
		} while(curVtx != rootLoopB);
		//__debugbreak();
		std::vector<unsigned int> vtxIdxToDebugAround;
		vtxIdxToDebugAround.push_back(vtxToEliminateOnAIdx);
		_mesh.SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAround, std::string("debug - ") + std::to_string(count) + std::string(" - loop1.obj"));
		vtxIdxToDebugAround.clear();
		vtxIdxToDebugAround.push_back(vtxToEliminateOnBIdx);
		_mesh.SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAround, std::string("debug - ") + std::to_string(count) + std::string(" - loop2.obj"));
		vtxIdxToDebugAround.clear();
		vtxIdxToDebugAround.push_back(vtxToEliminateOnAIdx);
		vtxIdxToDebugAround.push_back(vtxToEliminateOnBIdx);
		_mesh.SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAround, std::string("debug - ") + std::to_string(count) + std::string(" - loopBoth.obj"));
		vtxIdxToDebugAround.clear();
		curVtx = rootLoopA;
		do
		{
			vtxIdxToDebugAround.push_back(curVtx->GetVtxId());
			curVtx = curVtx->GetNext();
		} while(curVtx != rootLoopA);
		curVtx = rootLoopB;
		do
		{
			vtxIdxToDebugAround.push_back(curVtx->GetVtxId());
			curVtx = curVtx->GetNext();
		} while(curVtx != rootLoopB);
		_mesh.SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAround, std::string("debug - ") + std::to_string(count) + std::string(" - loopBeforeStich.obj"));
#endif // _DEBUG
	}
#endif // RETESS_DEBUGGING
	LoopElement const* curAVtx = rootLoopA;
	LoopElement const* curBVtx = rootLoopB;
	// Bresenham http://www.idav.ucdavis.edu/education/GraphicsNotes/Bresenhams-Algorithm.pdf
	float m = float(loopBLength) / float(loopALength);
	float e = m - 1.0f;
	bool bEdgeToStitch = true;
	do
	{
		StitchVtx(curAVtx->GetVtxId(), curAVtx->GetNext()->GetVtxId(), vtxToEliminateOnAIdx, curBVtx->GetVtxId());
		bEdgeToStitch = true;
		if(e >= 0.0f)
		{
			StitchVtx(curBVtx->GetVtxId(), loopAreSameDir ? curBVtx->GetNext()->GetVtxId() : curBVtx->GetPrev()->GetVtxId(), vtxToEliminateOnBIdx, curAVtx->GetNext()->GetVtxId());
			curBVtx = loopAreSameDir ? curBVtx->GetNext() : curBVtx->GetPrev();
			e -= 1.0f;
			bEdgeToStitch = false;
		}
		e += m;
		curAVtx = curAVtx->GetNext();
	} while(curAVtx != rootLoopA);
	if(bEdgeToStitch)
	{	// Stich last b segment if we have to
		StitchVtx(curBVtx->GetVtxId(), loopAreSameDir ? curBVtx->GetNext()->GetVtxId() : curBVtx->GetPrev()->GetVtxId(), vtxToEliminateOnBIdx, curAVtx->GetVtxId());
	}
	ASSERT(_mesh.IsVertexToBeRemoved(vtxToEliminateOnAIdx));
	ASSERT(_mesh.IsVertexToBeRemoved(vtxToEliminateOnBIdx));

#ifdef RETESS_DEBUGGING
	if(0)//count == 379)
	{
#ifdef _DEBUG
		// Check vertices
		LoopElement const* curVtx = rootLoopA;
		do
		{
			_mesh.CheckVertexIsCorrect(curVtx->GetVtxId());
			curVtx = curVtx->GetNext();
		} while(curVtx != rootLoopA);
		curVtx = rootLoopB;
		do
		{
			_mesh.CheckVertexIsCorrect(curVtx->GetVtxId());
			curVtx = curVtx->GetNext();
		} while(curVtx != rootLoopB);
		//
		std::vector<unsigned int> vtxIdxToDebugAround;
		curVtx = rootLoopA;
		do
		{
			vtxIdxToDebugAround.push_back(curVtx->GetVtxId());
			curVtx = curVtx->GetNext();
		} while(curVtx != rootLoopA);
		curVtx = rootLoopB;
		do
		{
			vtxIdxToDebugAround.push_back(curVtx->GetVtxId());
			curVtx = curVtx->GetNext();
		} while(curVtx != rootLoopB);
		_mesh.SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAround, std::string("debug - ") + std::to_string(count) + std::string(" - loopStiched.obj"));
#endif // _DEBUG
	}
#endif // RETESS_DEBUGGING

	// Treat degenerated edges, quad and triangles
	curAVtx = rootLoopA;
	do
	{
		if(!_mesh.IsVertexToBeRemoved(curAVtx->GetVtxId()))
		{
			// Remove degenerated triangles
			_mesh.DetectAndRemoveDegeneratedTrianglesAroundVertex(curAVtx->GetVtxId(), _newVerticesIDs);
			// Treat degenerated edges
			_mesh.DetectAndTreatDegeneratedEdgeAroundVertex(curAVtx->GetVtxId(), _newVerticesIDs);
			// Remove degenerated quads
			std::set<unsigned int> vtxAround;
			std::vector<unsigned int> const& triAround = _mesh.GetTrianglesAroundVertex(curAVtx->GetVtxId());
			for(unsigned int const& triIdx : triAround)
			{
				unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
				for(int j = 0; j < 3; ++j)
				{
					if(vtxsIdx[j] != curAVtx->GetVtxId())
						vtxAround.insert(vtxsIdx[j]);
				}
			}
			for(unsigned int vtxAroundIdx : vtxAround)
				_mesh.DetectAndRemoveDegeneratedQuadsAroundEdge(curAVtx->GetVtxId(), vtxAroundIdx);
		}
		curAVtx = curAVtx->GetNext();
	} while(curAVtx != rootLoopA);
	curBVtx = rootLoopB;
	do
	{
		if(!_mesh.IsVertexToBeRemoved(curBVtx->GetVtxId()))
			_mesh.DetectAndRemoveDegeneratedTrianglesAroundVertex(curBVtx->GetVtxId(), _newVerticesIDs);
		curBVtx = curBVtx->GetNext();
	} while(curBVtx != rootLoopB);

#ifdef RETESS_DEBUGGING
	if(0)//count == 379)
	{
#ifdef _DEBUG
		std::vector<unsigned int> vtxIdxToDebugAroundBefore;
		LoopElement const* curVtx = rootLoopA;
		do
		{
			vtxIdxToDebugAroundBefore.push_back(curVtx->GetVtxId());
			curVtx = curVtx->GetNext();
		} while(curVtx != rootLoopA);
		curVtx = rootLoopB;
		do
		{
			vtxIdxToDebugAroundBefore.push_back(curVtx->GetVtxId());
			curVtx = curVtx->GetNext();
		} while(curVtx != rootLoopB);
		_mesh.SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, std::string("debug - ") + std::to_string(count) + std::string(" - loopStichedAndCleaned.obj"));
#endif // _DEBUG
	}
#endif // RETESS_DEBUGGING

	// Recompute normals
	LoopElement const* curVtx = rootLoopA;
	do
	{
		_mesh.SetVertexAlreadyTreated(curVtx->GetVtxId());
		if(!_mesh.IsVertexToBeRemoved(curVtx->GetVtxId()))
			_mesh.SetHasToRecomputeNormal(curVtx->GetVtxId());
		curVtx = curVtx->GetNext();
	} while(curVtx != rootLoopA);
	curVtx = rootLoopB;
	do
	{
		_mesh.SetVertexAlreadyTreated(curVtx->GetVtxId());
		if(!_mesh.IsVertexToBeRemoved(curVtx->GetVtxId()))
			_mesh.SetHasToRecomputeNormal(curVtx->GetVtxId());
		curVtx = curVtx->GetNext();
	} while(curVtx != rootLoopB);
}