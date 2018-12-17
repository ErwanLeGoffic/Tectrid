#include "Mesh.h"
#include "SubMesh.h"
#include <float.h>
#include <time.h>
#include <map>
#include "OctreeVisitorGetIntersection.h"
#include "OctreeVisitorRecomputeBBox.h"
#include "OctreeVisitorExtractOutOfCellsBoundGeom.h"
#include "OctreeVisitorPurgeEmptyCell.h"
#include "OctreeVisitorHandlePendingRemovals.h"
#include "OctreeVisitorRetessellateInRange.h"
#include "Loop.h"
#include "..\SculptEngine.h"

#ifdef MESH_CONSISTENCY_CHECK
bool consistencyCheckOn = false;
#endif // MESH_CONSISTENCY_CHECK

bool VisitorGetOctreeVertexIntersection::HasToVisit(OctreeCell& cell)
{
	return _collider.Intersects(cell.GetContentBBox());
}

void VisitorGetOctreeVertexIntersection::VisitEnter(OctreeCell& cell)
{
	if(cell.GetTrianglesIdx().size() > 0) 	// leaf cell
		_collidedCells.push_back(&cell);
}

//#define DEBUG_ALWAYS_REBUILD_ALL_NORMALS
const float MINIMUM_MESH_SIDE_LENGTH = 100.0f;	// Ten centimeter
const unsigned int MAXIMUM_UNDO_COUNT = 10; // Limit the maximum snapshot stored, to avoid using too much memory

Mesh::Mesh(std::vector<unsigned int>& triangles, std::vector<Vector3>& vertices, int id, bool freeInputBuffers, bool rescale, bool recenter, bool buildHardEdges, bool weldVertices) : _id(id), _subMeshesVisitor(nullptr), _snapshotNextPos(0), _IsOpen(false), _IsManifold(true), _retessellator(nullptr)
{
	if(SculptEngine::HasExpired())
		return;
	// Transfer vertices and triangles and weld mingled vertices
	if(weldVertices)
	{
		WeldVertices(triangles, vertices, _triangles, _vertices);
		if(freeInputBuffers)
		{
			vertices.clear();
			vertices.shrink_to_fit();
			triangles.clear();
			triangles.shrink_to_fit();
		}
	}
	else
	{
		if(freeInputBuffers)
		{
			_vertices = std::move(vertices);
			_triangles = std::move(triangles);
		}
		else
		{
			_vertices = vertices;
			_triangles = triangles;
		}
	}
	RebuildMeshData(rescale, recenter, buildHardEdges);
#ifdef MESH_CONSISTENCY_CHECK
	CheckMeshIsCorrect();
#endif // MESH_CONSISTENCY_CHECK
	// Take snapshot so that we can undo up to the initial state
	TakeSnapShot();
}

Mesh::Mesh(Mesh const& otherMesh, bool copySnapshots, bool copyOctree):
	_vertices(otherMesh._vertices),
	_vtxsState(otherMesh._vtxsState),
	_vtxsNewIdx(otherMesh._vtxsNewIdx),
	_vtxsNormal(otherMesh._vtxsNormal),
	_vtxToTriAround(otherMesh._vtxToTriAround),
	_triangles(otherMesh._triangles),
	_trisState(otherMesh._trisState),
	_trisNewIdx(otherMesh._trisNewIdx),
	_trisNormal(otherMesh._trisNormal),
	_trisBSphere(otherMesh._trisBSphere),
	_vtxsIdxToRecomputeNormalOn(otherMesh._vtxsIdxToRecomputeNormalOn),
	_trisIdxToRecomputeNormalOn(otherMesh._trisIdxToRecomputeNormalOn),
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
	_vtxsIdxToRemove(otherMesh._vtxsIdxToRemove),
	_trisIdxToRemove(otherMesh._trisIdxToRemove),
	_vtxsIdxToRecycle(otherMesh._vtxsIdxToRecycle),
	_trisIdxToRecycle(otherMesh._trisIdxToRecycle),
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
	_id(otherMesh._id),
	_IsOpen(otherMesh._IsOpen),
	_IsManifold(otherMesh._IsManifold),
	_snapshotNextPos(0),
	_retessellator(nullptr)
{
	if(copySnapshots)
	{
		_verticesSnapshots = otherMesh._verticesSnapshots;
		_trianglesSnapshots = otherMesh._trianglesSnapshots;
		_snapshotNextPos = otherMesh._snapshotNextPos;
	}
	// Octree cloning
	if(copyOctree)
		_octreeRoot.reset(new OctreeCell(*otherMesh._octreeRoot, nullptr));
	// Submeshes
	if(otherMesh._subMeshesVisitor != nullptr)
		_subMeshesVisitor.reset(new VisitorBuildAndCollectSubMeshes(*this, *otherMesh._subMeshesVisitor));
}

void Mesh::RebuildMeshData(bool rescale, bool recenter, bool buildHardEdges)
{
	// Build for each vertex the list of the surrounding triangles
	_vtxToTriAround.clear();
	_vtxToTriAround.resize(_vertices.size());
	unsigned int triCount = (unsigned int) _triangles.size() / 3;
	for(unsigned int triIdx = 0; triIdx < triCount; ++triIdx)
	{
		unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
		for(int i = 0; i < 3; ++i)
			AddTriangleAroundVertex(vtxsIdx[i], triIdx);
	}
	// Create vertices state flag array
	_vtxsState.clear();
	_vtxsState.resize(_vertices.size());
	for(unsigned char& state : _vtxsState)
		AddStateFlags(state, VTX_STATE_HAS_TO_RECOMPUTE_NORMAL);
	// Create vertices new ID array
	_vtxsNewIdx.clear();
	_vtxsNewIdx.resize(_vertices.size());
	for(unsigned int& newIdx : _vtxsNewIdx)
		newIdx = UNDEFINED_NEW_ID;
	// Create triangles state flag array
	_trisState.clear();
	_trisState.resize(triCount);
	for(unsigned char& state : _trisState)
		AddStateFlags(state, TRI_STATE_HAS_TO_RECOMPUTE_NORMAL);
	// Create triangles new ID array
	_trisNewIdx.clear();
	_trisNewIdx.resize(triCount);
	for(unsigned int& newIdx : _trisNewIdx)
		newIdx = UNDEFINED_NEW_ID;
	// Create vertices normal array
	_vtxsNormal.clear();
	_vtxsNormal.resize(_vtxsState.size());
	// Create triangles normal array
	_trisNormal.clear();
	_trisNormal.resize(_trisState.size());
	// Create triangles bsphere array
	_trisBSphere.clear();
	_trisBSphere.resize(_trisState.size());
	// Reserve our reduced compute normals buffer
	_vtxsIdxToRecomputeNormalOn.clear();
	_vtxsIdxToRecomputeNormalOn.reserve(_vtxsNormal.size());
	_trisIdxToRecomputeNormalOn.clear();
	_trisIdxToRecomputeNormalOn.reserve(_trisNormal.size());
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
	// Reserve our pending Remove buffer
	_vtxsIdxToRemove.clear();
	_vtxsIdxToRecycle.clear();
	_vtxsIdxToRemove.reserve(_vtxsNormal.size());
	_vtxsIdxToRecycle.reserve(_vtxsNormal.size());
	_trisIdxToRemove.clear();
	_trisIdxToRecycle.clear();
	_trisIdxToRemove.reserve(_trisNormal.size());
	_trisIdxToRecycle.reserve(_trisNormal.size());
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
	// Compute bbox
	BBox bbox;
	for(Vector3 const& vertex : _vertices)
		bbox.Encapsulate(vertex);
	// Center object on world origin, to make mirroring work	// Todo: Find a way to make symmetry work on non center objects
	if(recenter)
	{
		Vector3 bBoxRecenterVector(bbox.Center());
		bBoxRecenterVector.Negate();
		if(bBoxRecenterVector.LengthSquared() > 0.0f)
		{
			for(Vector3& vertex : _vertices)
				vertex += bBoxRecenterVector;
			bbox.Translate(bBoxRecenterVector);
		}
	}
	// Test if the object is not too small, if so, scale it : For example that file "C:\data\from clara.io\head-scan.obj" was 0.5 unit wide, when we deeply tessellated it, collisions tests were failing
	if(rescale)
	{
		Vector3 bBoxSize = bbox.Size();
		float minSideLength = min(bBoxSize.x, bBoxSize.y);
		minSideLength = min(minSideLength, bBoxSize.z);
		float limitOverMin = MINIMUM_MESH_SIDE_LENGTH / minSideLength;
		if(limitOverMin > 1.0f)
		{	// Have to scale the mesh up to the minimum threshold
			for(Vector3& vertex : _vertices)
				vertex *= limitOverMin;
			bbox.Scale(limitOverMin);
		}
	}
	// Compute normals
	RecomputeNormals(false);
	// Identify open edges
	TagAndCollectOpenEdgesVertices(nullptr);
	// Build hard edges
	if(buildHardEdges)
	{
		RemoveFlatTriangles();
		ProcessHardEdges(nullptr, nullptr);
		RecomputeNormals(false);	// Todo: could prevent redundant normals compute
	}
	// Build octree
	if(bbox.IsValid())
		BuildOctree(bbox);
	printf("triangle count %d, vertex count %d\n", (int) _triangles.size() / 3, (int) _vertices.size());
}

bool Mesh::GetClosestIntersectionPoint(Ray const& ray, Vector3& point, Vector3* normal, bool cullBackFace)
{
	unsigned int triIdx = 0;
	return GetClosestIntersectionPointAndTriangle(ray, point, &triIdx, normal, cullBackFace);
}

bool Mesh::GetClosestIntersectionPointAndTriangle(Ray const& ray, Vector3& point, unsigned int* triangle, Vector3* normal, bool cullBackFace)
{
	if(_octreeRoot == nullptr)
		return false;
	// Get triangle we intersect with
	DEBUG_intersectionPoints.clear();

	VisitorGetIntersection getIntersection(*this, ray, cullBackFace, false);
	GrabOctreeRoot().Traverse(getIntersection);
	std::vector<float> const& intersectionDists = getIntersection.GetIntersectionDists();
	std::vector<unsigned int> const& intersectionTriangles = getIntersection.GetIntersectionTriangles();
	ASSERT(intersectionDists.size() == intersectionTriangles.size());

	// Get Closest intersection
	if(intersectionDists.size() > 0)
	{
		float closestDist = FLT_MAX;
		for(size_t i = 0; i < intersectionDists.size(); ++i)
		{
			if(closestDist > intersectionDists[i])
			{
				closestDist = intersectionDists[i];
				if(triangle != nullptr)
					*triangle = intersectionTriangles[i];
				if(normal != nullptr)
					*normal = _trisNormal[intersectionTriangles[i]];
			}
		}
		point = ray.GetOrigin() + ray.GetDirection() * closestDist;
		return true;
	}

	point.ResetToZero();
	return false;
}

BBox const& Mesh::GetBBox() const
{
	if(_octreeRoot != nullptr)
		return _octreeRoot->GetContentBBox();
	else
	{
		static BBox emptyBBox(Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f));
		return emptyBBox;
	}
}

void Mesh::RecomputeFragmentsBBox(bool forceAllCellRecomputing)
{
	VisitorRecomputeBBox recomputeBbox(*this, forceAllCellRecomputing);
	GrabOctreeRoot().Traverse(recomputeBbox);
}

void Mesh::ReBalanceOctree(std::vector<unsigned int> const& additionalTrisToInsert, std::vector<unsigned int> const& additionalVtxsToInsert, bool extractFromAllCells)
{
	if(_octreeRoot == nullptr)
		return;
	// Extract out of cells bound triangles and vertices
	VisitorExtractOutOfCellsBoundGeom extractOutOfBounds(*this, extractFromAllCells);
	GrabOctreeRoot().Traverse(extractOutOfBounds);
	// Insert them back into the octree
	if((additionalTrisToInsert.size() > 0) || (additionalVtxsToInsert.size() > 0))
	{
		std::vector<unsigned int> allTriToInsert;
		allTriToInsert.reserve(extractOutOfBounds.GetExtractedTris().size() + additionalTrisToInsert.size());
		allTriToInsert.insert(allTriToInsert.end(), extractOutOfBounds.GetExtractedTris().begin(), extractOutOfBounds.GetExtractedTris().end());
		allTriToInsert.insert(allTriToInsert.end(), additionalTrisToInsert.begin(), additionalTrisToInsert.end());
		std::vector<unsigned int> allVtxsToInsert;
		allVtxsToInsert.reserve(extractOutOfBounds.GetExtractedVtxs().size() + additionalVtxsToInsert.size());
		allVtxsToInsert.insert(allVtxsToInsert.end(), extractOutOfBounds.GetExtractedVtxs().begin(), extractOutOfBounds.GetExtractedVtxs().end());
		allVtxsToInsert.insert(allVtxsToInsert.end(), additionalVtxsToInsert.begin(), additionalVtxsToInsert.end());
		GrabOctreeRoot().Insert(*this, allTriToInsert, allVtxsToInsert);
	}
	else
		GrabOctreeRoot().Insert(*this, extractOutOfBounds.GetExtractedTris(), extractOutOfBounds.GetExtractedVtxs());
	// Purge empty cells
	VisitorPurgeEmptyCell purgeEmptyCell;
	GrabOctreeRoot().Traverse(purgeEmptyCell);
	// Recompute bboxes
	RecomputeFragmentsBBox(extractFromAllCells);
}

void Mesh::HandlePendingRemovals()
{	// Will actually remove vertices and triangles that were tagged as "remove pending" and will pack again vertices and triangles data
#ifdef THICKNESS_HANDLER_WIP
	//return;	// temporory, not to be bothered by index remapping
#endif // THICKNESS_HANDLER_WIP
	// First will flag all vertex and triangles that will have to be moved at the end of removal process
	unsigned int backPosOfMovedElement = (unsigned int) _vtxsState.size() - 1;
	for(signed int i = (signed int) _vtxsState.size() - 1; i >= 0 ; --i)
	{
		if(TestStateFlags(_vtxsState[i], VTX_STATE_PENDING_REMOVE))
		{
			if((unsigned int) i < backPosOfMovedElement)
			{
				unsigned int j;
				for(j = (signed int) _vtxsState.size() - 1; j > backPosOfMovedElement; --j)
				{
					if(_vtxsNewIdx[j] == (unsigned int) backPosOfMovedElement)
						break;	// Means we move again an element we have already moved
				}
				_vtxsNewIdx[j] = i;	// Note: all new ids are stored at the end of the array, thus we won't have to clear the data, it will be done during the pop_backs
				--backPosOfMovedElement;
			}
			else
				--backPosOfMovedElement;	// Element to remove is already at the end of the array, we won't have to move anything for that element
		}
	}
	backPosOfMovedElement = (unsigned int) _trisState.size() - 1;
	for(signed int i = (signed int) _trisState.size() - 1; i >= 0; --i)
	{
		if(TestStateFlags(_trisState[i], TRI_STATE_PENDING_REMOVE))
		{
			if((unsigned int) i < backPosOfMovedElement)
			{
				unsigned int j;
				for(j = (signed int) _trisState.size() - 1; j > backPosOfMovedElement; --j)
				{
					if(_trisNewIdx[j] == (unsigned int) backPosOfMovedElement)
						break;	// Means we move again an element we have already moved
				}
				_trisNewIdx[j] = i;	// Note: all new ids are stored at the end of the array, thus we won't have to clear the data, it will be done during the pop_backs
				--backPosOfMovedElement;
			}
			else
				--backPosOfMovedElement;	// Element to remove is already at the end of the array, we won't have to move anything for that element
		}
	}
	// Purge from octree vertices and triangles that were set to be removed and remap the vertices and triangles index of the one that will be moved
	if(_octreeRoot != nullptr)
	{
		OctreeVisitorHandlePendingRemovals handlePendingRemovals(*this, true);
		_octreeRoot->Traverse(handlePendingRemovals);
	}
	// Do the same on the retessellate component
	GrabRetessellator().HandlePendingRemovals();
	// Update in the same way _vtxToTriAround data
	for(unsigned int vtxIdx = 0; vtxIdx < _vtxToTriAround.size(); ++vtxIdx)
	{
		if(TestStateFlags(_vtxsState[vtxIdx], VTX_STATE_PENDING_REMOVE) == false)
		{
			std::vector<unsigned int>& trianglesIdx = _vtxToTriAround[vtxIdx];
			for(unsigned int i = 0; i < trianglesIdx.size();)
			{
				unsigned int& triIdx = trianglesIdx[i];
				if(TestStateFlags(_trisState[triIdx], TRI_STATE_PENDING_REMOVE))
				{	// Remove element
					triIdx = trianglesIdx.back();
					trianglesIdx.pop_back();
					ASSERT(trianglesIdx.size() > 2);	// Must be the case on a manifold mesh
				}
				else
				{
					if(TriHasToMove(triIdx))
						triIdx = GetNewTriIdx(triIdx);	// Remap element
					++i;
				}
			}
		}
	}
	// Update _triangles data
	for(unsigned int i = 0; i < _triangles.size(); ++i)
	{
		unsigned int& vtxIdx = _triangles[i];
		if(VtxHasToMove(vtxIdx))
		{	// Remap element
			vtxIdx = GetNewVtxIdx(vtxIdx);
		}
		else
			ASSERT((TestStateFlags(_vtxsState[vtxIdx], VTX_STATE_PENDING_REMOVE) == false) || TestStateFlags(_trisState[i / 3], TRI_STATE_PENDING_REMOVE));	// Should never happen: when someone removes a vertex he should relink the affected triangles to a new vertex
	}
	// Update _trisIdxToRecomputeNormalOn data
	for(unsigned int i = 0; i < _trisIdxToRecomputeNormalOn.size();)
	{
		unsigned int& triIdx = _trisIdxToRecomputeNormalOn[i];
		if(TestStateFlags(_trisState[triIdx], TRI_STATE_PENDING_REMOVE))
		{	// Remove element
			triIdx = _trisIdxToRecomputeNormalOn.back();
			_trisIdxToRecomputeNormalOn.pop_back();
		}
		else
		{
			if(TriHasToMove(triIdx))
				triIdx = GetNewTriIdx(triIdx);// Remap element
			++i;
		}
	}
	// Update _vtxsIdxToRecomputeNormalOn data
	for(unsigned int i = 0; i < _vtxsIdxToRecomputeNormalOn.size();)
	{
		unsigned int& vtxIdx = _vtxsIdxToRecomputeNormalOn[i];
		if(TestStateFlags(_vtxsState[vtxIdx], VTX_STATE_PENDING_REMOVE))
		{	// Remove element
			vtxIdx = _vtxsIdxToRecomputeNormalOn.back();
			_vtxsIdxToRecomputeNormalOn.pop_back();
		}
		else
		{	
			if(VtxHasToMove(vtxIdx))
				vtxIdx = GetNewVtxIdx(vtxIdx);	// Remap element
			++i;
		}
	}
	// Now do the main vertices and triangles data removal
	for(signed int i = (signed int) _vtxsState.size() - 1; i >= 0; --i)
	{
		if(TestStateFlags(_vtxsState[i], VTX_STATE_PENDING_REMOVE))
		{
			_vertices[i] = _vertices.back();
			_vertices.pop_back();
			_vtxsNormal[i] = _vtxsNormal.back();
			_vtxsNormal.pop_back();
			_vtxToTriAround[i] = _vtxToTriAround.back();
			_vtxToTriAround.pop_back();
			_vtxsState[i] = _vtxsState.back();
			_vtxsState.pop_back();
			_vtxsNewIdx.pop_back();
		}
	}
	for(signed int i = (signed int) _trisState.size() - 1; i >= 0; --i)
	{
		if(TestStateFlags(_trisState[i], TRI_STATE_PENDING_REMOVE))
		{
			_triangles[i * 3 + 2] = _triangles.back();
			_triangles.pop_back();
			_triangles[i * 3 + 1] = _triangles.back();
			_triangles.pop_back();
			_triangles[i * 3] = _triangles.back();
			_triangles.pop_back();
			_trisNormal[i] = _trisNormal.back();
			_trisNormal.pop_back();
			_trisBSphere[i] = _trisBSphere.back();
			_trisBSphere.pop_back();
			_trisState[i] = _trisState.back();
			_trisState.pop_back();
			_trisNewIdx.pop_back();
		}
	}
	// Purge empty cells
	if(_octreeRoot != nullptr)
	{
		VisitorPurgeEmptyCell purgeEmptyCell;
		_octreeRoot->Traverse(purgeEmptyCell);
	}
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
	// Clear our pending Remove buffer
	_vtxsIdxToRemove.clear();
	_vtxsIdxToRecycle.clear();
	_trisIdxToRemove.clear();
	_trisIdxToRecycle.clear();
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
}

void Mesh::RecomputeNormals(bool forceReducedCompute, bool authorizeAutoSmooth)
{
#ifdef DEBUG_ALWAYS_REBUILD_ALL_NORMALS
	// Reset vertices normal
	for(int i = 0; i < _vertices.size(); ++i)
		_vtxsNormal[i].ResetToZero();

	// Build triangles and vertices normal
	for(int i = 0; i < _trisState.size(); ++i)
	{
		// Compute triangles normal
		unsigned int const* vtxsIdx = &(_triangles[i * 3]);

		Vector3& vtx1 = _vertices[vtxsIdx[0]];
		Vector3& vtx2 = _vertices[vtxsIdx[1]];
		Vector3& vtx3 = _vertices[vtxsIdx[2]];

		Vector3 p1p2 = vtx2 - vtx1;
		Vector3 p1p3 = vtx3 - vtx1;

		Vector3& triNormal = _trisNormal[i];
		triNormal = p1p2.Cross(p1p3);
		float triNormalLength = triNormal.Length();
		if(triNormalLength != 0.0f)
		{
			triNormal /= triNormalLength;
			if(SculptEngine::IsTriangleOrientationInverted())
				triNormal.Negate();
		}

		// (Also compute triangle bsphere)
		_trisBSphere[i].BuildFromTriangle(vtx1, vtx2, vtx3);

		// sum normal on triangle vertices
		_vtxsNormal[vtxsIdx[0]] += triNormal;
		_vtxsNormal[vtxsIdx[1]] += triNormal;
		_vtxsNormal[vtxsIdx[2]] += triNormal;
	}

	// Normalize vertices normal
	for(int i = 0; i < _vertices.size(); ++i)
		_vtxsNormal[i].Normalize();

	// Empty reduced normal computing vectors
	_vtxsIdxToRecomputeNormalOn.clear();
	_trisIdxToRecomputeNormalOn.clear();
	return;
#else
	if((_vtxsIdxToRecomputeNormalOn.size() == 0) && (forceReducedCompute == false))	// Full recompute of the normals' mesh
	{
		// Reset vertices normal
#pragma omp parallel for
		for(int i = 0; i < _vertices.size(); ++i)
			_vtxsNormal[i].ResetToZero();

		// Build triangles and vertices normal
#pragma omp parallel for
		for(int i = 0; i < _trisState.size(); ++i)
		{
			// Compute triangles normal
			unsigned char& triState = _trisState[i];
			unsigned int const* vtxsIdx = &(_triangles[i * 3]);

			Vector3& vtx1 = _vertices[vtxsIdx[0]];
			Vector3& vtx2 = _vertices[vtxsIdx[1]];
			Vector3& vtx3 = _vertices[vtxsIdx[2]];

			Vector3 p1p2 = vtx2 - vtx1;
			Vector3 p1p3 = vtx3 - vtx1;

			Vector3& triNormal = _trisNormal[i];
			triNormal = p1p2.Cross(p1p3);
			float triNormalLength = triNormal.Length();
			if(triNormalLength != 0.0f)
			{
				triNormal /= triNormalLength;
				if(SculptEngine::IsTriangleOrientationInverted())
					triNormal.Negate();
			}

			// (Also compute triangle bsphere)
			_trisBSphere[i].BuildFromTriangle(vtx1, vtx2, vtx3);

			// sum normal on triangle vertices
			_vtxsNormal[vtxsIdx[0]] += triNormal;
			_vtxsNormal[vtxsIdx[1]] += triNormal;
			_vtxsNormal[vtxsIdx[2]] += triNormal;

			ClearStateFlags(triState, TRI_STATE_HAS_TO_RECOMPUTE_NORMAL);
		}

		// Normalize vertices normal
#pragma omp parallel for
		for(int i = 0; i < _vertices.size(); ++i)
		{
			if (_vtxsNormal[i].LengthSquared() == 0.0f)	// Todo: remove this, we shouldn't end up with null vtx normals
			{
				if (!_vtxToTriAround[i].empty())
					_vtxsNormal[i] = _trisNormal[_vtxToTriAround[i][0]];
				else
					_vtxsNormal[i] = Vector3(1.0f, 0.0f, 0.0f);
			}
			_vtxsNormal[i].Normalize();
			ClearStateFlags(_vtxsState[i], VTX_STATE_HAS_TO_RECOMPUTE_NORMAL);
		}
	}
	else // Reduced recompute of the normals' mesh
	{
		// Update triangles normal
#pragma omp parallel for
		for(int i = 0; i < _trisIdxToRecomputeNormalOn.size(); ++i)
		{
			unsigned int const& triIdx = _trisIdxToRecomputeNormalOn[i];
			// Compute triangles normal
			unsigned char& triState = _trisState[triIdx];
			if(TestStateFlags(triState, TRI_STATE_HAS_TO_RECOMPUTE_NORMAL))
			{
				unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);

				// If a triangle has to recompute its normal, its vertices must have been set the same way
				ASSERT(TestStateFlags(_vtxsState[vtxsIdx[0]], VTX_STATE_HAS_TO_RECOMPUTE_NORMAL));
				ASSERT(TestStateFlags(_vtxsState[vtxsIdx[1]], VTX_STATE_HAS_TO_RECOMPUTE_NORMAL));
				ASSERT(TestStateFlags(_vtxsState[vtxsIdx[2]], VTX_STATE_HAS_TO_RECOMPUTE_NORMAL));

				Vector3& vtx1 = _vertices[vtxsIdx[0]];
				Vector3& vtx2 = _vertices[vtxsIdx[1]];
				Vector3& vtx3 = _vertices[vtxsIdx[2]];

				Vector3 p1p2 = vtx2 - vtx1;
				Vector3 p1p3 = vtx3 - vtx1;

				Vector3& triNormal = _trisNormal[triIdx];
				triNormal = p1p2.Cross(p1p3);
				float triNormalLength = triNormal.Length();
				if(triNormalLength != 0.0f)
				{
					triNormal /= triNormalLength;
					if(SculptEngine::IsTriangleOrientationInverted())
						triNormal.Negate();
				}

				// (Also compute triangle bsphere)
				_trisBSphere[triIdx].BuildFromTriangle(vtx1, vtx2, vtx3);

				ClearStateFlags(triState, TRI_STATE_HAS_TO_RECOMPUTE_NORMAL);
			}
		}

		// Update vertices normal
		bool autoSmooth = authorizeAutoSmooth && HasToDoAnEmergencyAutoSmooth();
#pragma omp parallel for
		for(int i = 0; i < _vtxsIdxToRecomputeNormalOn.size(); ++i)
		{
			unsigned int const& vtxIdx = _vtxsIdxToRecomputeNormalOn[i];
			unsigned char& vtxState = _vtxsState[vtxIdx];
			if(TestStateFlags(vtxState, VTX_STATE_HAS_TO_RECOMPUTE_NORMAL))
			{
				Vector3& vtxNormal = _vtxsNormal[vtxIdx];
				std::vector<unsigned int> const& triArround = _vtxToTriAround[vtxIdx];
				if(triArround.empty() == false)
				{
					vtxNormal = _trisNormal[triArround[0]];
					// Sum normal from all surrounding faces
					for(unsigned int j = 1; j < (unsigned int) triArround.size(); ++j)
						vtxNormal += _trisNormal[triArround[j]];

					if(autoSmooth)
					{	// AutoSmooth, still experimental
						Vector3 averageVertex;
						unsigned int nbVerticesInvolved = 0;
						for(unsigned int triIdx : _vtxToTriAround[vtxIdx])
						{
							unsigned int const* vtxIdxs = &(_triangles[triIdx * 3]);
							for(unsigned int k = 0; k < 3; ++k)
							{
								if(vtxIdxs[k] != vtxIdx)
								{
									averageVertex += _vertices[vtxIdxs[k]];
									nbVerticesInvolved++;
								}
							}
						}
						_vertices[vtxIdx] = averageVertex / float(nbVerticesInvolved);
					}

					if(vtxNormal.LengthSquared() == 0.0f)	// Todo: remove this, we shouldn't end up with null vtx normals
						vtxNormal = _trisNormal[triArround[0]];
					vtxNormal.Normalize();
				}
				ClearStateFlags(vtxState, VTX_STATE_HAS_TO_RECOMPUTE_NORMAL);
			}
		}
	}
	// Empty reduced normal computing vectors
	_vtxsIdxToRecomputeNormalOn.clear();
	_trisIdxToRecomputeNormalOn.clear();
#endif // DEBUG_ALWAYS_REBUILD_ALL_NORMALS
}

void Mesh::ProcessHardEdges(std::vector<unsigned int>* createdTriangles, std::vector<unsigned int>* createdVertices)
{
#ifdef MESH_CONSISTENCY_CHECK
	CheckMeshIsCorrect();
#endif // MESH_CONSISTENCY_CHECK
#ifdef PROFILE_INFO
	printf("Start processing hard edges...\n");
	printf("Before hard edge processing: triangle count %d, vertex count %d\n", (int) _triangles.size() / 3, (int) _vertices.size());
	clock_t begin = clock();
#endif // PROFILE_INFO	
	ASSERT(_trisIdxToRecycle.size() == 0);	// We assume there is no triangle to recycle before doing this
	std::vector<unsigned int> smoothGroup(_trisState.size());
	for(unsigned int& elem : smoothGroup)
		elem = UNDEFINED_NEW_ID;
	// Identify triangles that touches an hard edge. This is important to treat only those triangles. If we treat all polygons, then a smoothgroup could leak to other place on the mesh by propagating if the hard edge is not a complete loop, so has at some places no hard edge, on which the smooth group propagation will jump into, putting all the smooth group work to waste.
	const unsigned int TRI_TO_TREAT = UNDEFINED_NEW_ID - 1;
	for(unsigned int i = 0; i < _vertices.size(); ++i)
	{
		std::vector<unsigned int> const& trisAround = _vtxToTriAround[i];
		if(trisAround.size() >= 2)
		{
			Vector3 refNormal = _trisNormal[trisAround[0]];
			bool haveToIncludeInSmoothGroup = false;
			for(unsigned int j = 1; (j < trisAround.size()) && !haveToIncludeInSmoothGroup; ++j)
			{
				unsigned int triAround = trisAround[j];
				if(refNormal.Dot(_trisNormal[triAround]) < smoothGroupCosLimit)
				{
					haveToIncludeInSmoothGroup = true;
					break;
				}
			}
			if(haveToIncludeInSmoothGroup)
			{
				for(unsigned int triAround : trisAround)
					smoothGroup[triAround] = TRI_TO_TREAT;
			}
		}
	}
	// Identity the group of each triangle
	unsigned int lastTriTreated = 0;
	unsigned int curSmoothGroup = 0;	// Be aware that group 0 is for null normal / degenerated triangles
	std::vector<std::vector<unsigned int>> triInEachSmoothGroup(1);
	while(lastTriTreated < smoothGroup.size())
	{
		// Position to next smooth group to build
		bool bFoundTriToTreat = false;
		while((lastTriTreated < smoothGroup.size()) && !bFoundTriToTreat)
		{
			while((lastTriTreated < smoothGroup.size()) && (smoothGroup[lastTriTreated] != TRI_TO_TREAT))
				++lastTriTreated;
			while((lastTriTreated < smoothGroup.size()) && (_trisNormal[lastTriTreated].LengthSquared() == 0.0f))
			{
				smoothGroup[lastTriTreated] = 0;	// 0 group is for null normal / degenerated triangles
				triInEachSmoothGroup[0].push_back(lastTriTreated);
				++lastTriTreated;
			}
			if((lastTriTreated < smoothGroup.size()) && (smoothGroup[lastTriTreated] == TRI_TO_TREAT))
				bFoundTriToTreat = true;
		}
		if(bFoundTriToTreat)
		{
			++curSmoothGroup;
			triInEachSmoothGroup.push_back(std::vector<unsigned int>());
			// Propagate smooth group inclusion until there is no surrounding triangles to treat (thus smooth group is complete)
			std::set<unsigned int> trisToTreat;
			trisToTreat.insert(lastTriTreated);
			smoothGroup[lastTriTreated] = curSmoothGroup;
			triInEachSmoothGroup[curSmoothGroup].push_back(lastTriTreated);
			while(trisToTreat.empty() == false)
			{
				unsigned int triToTreat = *(trisToTreat.begin());
				Vector3 const& refNormal = _trisNormal[triToTreat];
				trisToTreat.erase(trisToTreat.begin());
				unsigned int const* triToTreatVtxIdxs = &(_triangles[triToTreat * 3]);
				// Prevent the smooth group to propagate on the other side of the hard edge
				bool processSurroundings = true;
				for(unsigned int i = 0; i < 3; ++i)
				{
					std::vector<unsigned int> const& trisAround = _vtxToTriAround[triToTreatVtxIdxs[i]];
					for(unsigned int triAround : trisAround)
					{
						if((triAround != triToTreat) && (smoothGroup[triAround] == curSmoothGroup))
						{
							if(refNormal.Dot(_trisNormal[triAround]) < smoothGroupCosLimit)
								processSurroundings = false;
						}
					}
				}
				// Include all surrounding triangle if they fit in the smooth group
				if(processSurroundings)
				{
					for(unsigned int i = 0; i < 3; ++i)
					{
						std::vector<unsigned int> const& trisAround = _vtxToTriAround[triToTreatVtxIdxs[i]];
						for(unsigned int triAround : trisAround)
						{
							if((triAround != triToTreat) && (smoothGroup[triAround] == TRI_TO_TREAT))
							{
								if(refNormal.Dot(_trisNormal[triAround]) >= smoothGroupCosLimit)
								{	// Included in smooth group
									trisToTreat.insert(triAround);
									smoothGroup[triAround] = curSmoothGroup;
									triInEachSmoothGroup[curSmoothGroup].push_back(triAround);
								}
							}
						}
					}
				}
			}
		}
	}
	// Now will create flat triangles around each smooth group to isolate them from the other smooth group (while keeping the connectivity)	
	for(curSmoothGroup = 1; curSmoothGroup < triInEachSmoothGroup.size(); ++curSmoothGroup)	// Start at smoothgroup 1 as we don't update the group 0 that holds degenerated triangles
	{
		ClearAllVerticesAlreadyTreated(); // Clear vertex treated on all vertex, as we will need this to prevent redundand treatment
		for(unsigned int triToTreat : triInEachSmoothGroup[curSmoothGroup])
		{
			for(unsigned int i = 0; i < 3; ++i)
			{
				unsigned int vtxToTreat = _triangles[triToTreat * 3 + i];
				if(!IsVertexAlreadyTreated(vtxToTreat))
				{
					bool allSurroundingTrisAreFromTheSameSmoothGroup = true;
					for(unsigned int triAround : _vtxToTriAround[vtxToTreat])
					{
						if((smoothGroup[triAround] != 0) && (smoothGroup[triAround] != UNDEFINED_NEW_ID) && (smoothGroup[triAround] != curSmoothGroup))
						{
							allSurroundingTrisAreFromTheSameSmoothGroup = false;
							break;
						}
					}
					if(!allSurroundingTrisAreFromTheSameSmoothGroup)
					{	// Have to create degenerated triangles to isolate the smooth group
						bool smoothGroupTrianglesAroundVertexAreJoinedTogether = true;
						// First we determine if the smooth group triangles are joined together. If not, we won't go any further... too complex to handle.
						// By "joined together", I mean there is not a triangle of another smooth group splitting the smoothgroup triangles we are treating.
						// For example imagine we are treating smooth group number 3, we run through the triangles around a vertex in a clockwise order and looking at their smoothgroup number, we would get this: 0, 3, 3, 5, 3, 6
						// This case would be a mess to handle, for the moment I have no idea, as this won't happen very often, I think it's ok no to treat it, shouldn't make a noticeable visual difference.
						// Todo: This is a temp solution (see explanation above), find a more robust way. Or at least try to optimize this copy/paste of the loop system twice
						unsigned int genTriangles = 0;
						for(unsigned int triAroundIndex = 0; triAroundIndex < _vtxToTriAround[vtxToTreat].size(); ++triAroundIndex)
						{
							unsigned int triAround = _vtxToTriAround[vtxToTreat][triAroundIndex];
							if(smoothGroup[triAround] == curSmoothGroup)
							{
								for(unsigned int j = 0; j < 3; ++j)
								{
									unsigned int* vtxIdxsTriAround = &(_triangles[triAround * 3]);
									if(vtxIdxsTriAround[j] != vtxToTreat)
									{	// Check if the triangles touching edge vtxIdxsTriAround[j] <-> vtxToTreat are on the curSmoothGroup
										unsigned int idxA = vtxToTreat;
										unsigned int idxB = vtxIdxsTriAround[j];
										std::vector<unsigned int> const& triAroundA = _vtxToTriAround[idxA];
										std::vector<unsigned int> const& triAroundB = _vtxToTriAround[idxB];
										bool limitEdgeTreated = false;
										for(unsigned int triIdxA : triAroundA)		// Todo : O(n), of course optimize this
										{
											for(unsigned int triIdxB : triAroundB)
											{
												if(triIdxA == triIdxB)
												{	// triangle common to vertex A and B
													if((smoothGroup[triIdxA] != curSmoothGroup))
													{	// Edge on the smooth group limit
														++genTriangles;
														limitEdgeTreated = true;
													}
													break;
												}
											}
											if(limitEdgeTreated)
												break;
										}
									}
								}
							}
						}
						smoothGroupTrianglesAroundVertexAreJoinedTogether = genTriangles <= 2;
						if(smoothGroupTrianglesAroundVertexAreJoinedTogether)
						{
							// Create the additional vertex
							unsigned int newVtx = AddVertex(_vertices[vtxToTreat]);
							if(IsVertexOnOpenEdge(vtxToTreat))
								SetVertexOnOpenEdge(newVtx);
							if(createdVertices != nullptr)
								createdVertices->push_back(newVtx);
							for(unsigned int triAroundIndex = 0; triAroundIndex < _vtxToTriAround[vtxToTreat].size(); )
							{
								unsigned int triAround = _vtxToTriAround[vtxToTreat][triAroundIndex];
								if(smoothGroup[triAround] == curSmoothGroup)
								{
									for(unsigned int j = 0; j < 3; ++j)
									{
										unsigned int* vtxIdxsTriAround = &(_triangles[triAround * 3]);
										if(vtxIdxsTriAround[j] != vtxToTreat)
										{	// Check if the triangles touching edge vtxIdxsTriAround[j] <-> vtxToTreat are on the curSmoothGroup
											unsigned int idxA = vtxToTreat;
											unsigned int idxB = vtxIdxsTriAround[j];
											std::vector<unsigned int> const& triAroundA = _vtxToTriAround[idxA];
											std::vector<unsigned int> const& triAroundB = _vtxToTriAround[idxB];
											bool limitEdgeTreated = false;
											for(unsigned int triIdxA : triAroundA)		// Todo : O(n), of course optimize this
											{
												for(unsigned int triIdxB : triAroundB)
												{
													if(triIdxA == triIdxB)
													{	// triangle common to vertex A and B
														if((smoothGroup[triIdxA] != curSmoothGroup))
														{	// Edge on the smooth group limit
															Vector3 nullNorm;
															bool revertTri = false;
															unsigned int* vtxIdxsTriA = &(_triangles[triIdxA * 3]);
															if(vtxIdxsTriA[0] == idxA)
																revertTri = vtxIdxsTriA[2] == idxB;
															else if(vtxIdxsTriA[1] == idxA)
																revertTri = vtxIdxsTriA[0] == idxB;
															else //if(vtxIdxsTriA[2] == idxA)
															{
																ASSERT(vtxIdxsTriA[2] == idxA);
																revertTri = vtxIdxsTriA[1] == idxB;
															}
															unsigned int newTriIDx = UNDEFINED_NEW_ID;
															if(revertTri)
																newTriIDx = AddTriangle(idxA, idxB, newVtx, nullNorm, true);
															else
																newTriIDx = AddTriangle(newVtx, idxB, idxA, nullNorm, true);
															AddTriangleAroundVertex(idxA, newTriIDx);
															AddTriangleAroundVertex(idxB, newTriIDx);
															AddTriangleAroundVertex(newVtx, newTriIDx);
															if(createdTriangles != nullptr)
																createdTriangles->push_back(newTriIDx);
															ASSERT(smoothGroup.size() == newTriIDx);
															smoothGroup.push_back(0);	// Degenerated triangle, so smooth group 0
															limitEdgeTreated = true;
														}
														break;
													}
												}
												if(limitEdgeTreated)
													break;
											}
										}
										else
											vtxIdxsTriAround[j] = newVtx;	// Triangle vertex we are duplicating, change "vtx around" data to the newly created point
									}
									RemoveTriangleAroundVertex(vtxToTreat, triAround);
									ASSERT((_vtxToTriAround[vtxToTreat].size() > 2) || IsVertexOnOpenEdge(vtxToTreat));
									AddTriangleAroundVertex(newVtx, triAround);
								}
								else
									++triAroundIndex;
							}
						}
					}
					SetVertexAlreadyTreated(vtxToTreat);
#ifdef MESH_CONSISTENCY_CHECK
					CheckVertexIsCorrect(vtxToTreat);
#endif // MESH_CONSISTENCY_CHECK
				}
			}
		}
	}
	ClearAllVerticesAlreadyTreated();
#ifdef PROFILE_INFO
	clock_t end = clock();
	printf("After hard edge processing: triangle count %d, vertex count %d\n", (int) _triangles.size() / 3, (int) _vertices.size());
	printf("Hard edges handled in %f\n", double(end - begin) / CLOCKS_PER_SEC);
#endif // PROFILE_INFO
#ifdef MESH_CONSISTENCY_CHECK
	CheckMeshIsCorrect();
#endif // MESH_CONSISTENCY_CHECK
}

void Mesh::SetHasToRecomputeNormal(unsigned int vertexIndex)
{
#ifdef MESH_CONSISTENCY_CHECK
	CheckVertexIsCorrect(vertexIndex);
#endif // MESH_CONSISTENCY_CHECK
	// Set flag on the vertex itself, and on the surrounding triangles, and the triangle's vertices
	unsigned char& vtxState = _vtxsState[vertexIndex];
	ASSERT(!TestStateFlags(vtxState, VTX_STATE_PENDING_REMOVE));
	if(!TestStateFlags(vtxState, VTX_STATE_HAS_TO_RECOMPUTE_NORMAL))
	{
		_vtxsIdxToRecomputeNormalOn.push_back(vertexIndex);
		AddStateFlags(vtxState, VTX_STATE_HAS_TO_RECOMPUTE_NORMAL | VTX_STATE_HAS_TO_RECOMPUTE_SUB_MESH);
	}
	// Even if the vertex was already flagged to recompute normal, run on its surrounding triangles. As this vertex could have been flagged when handling surrounding triangles before this one
	std::vector<unsigned int> const& triArround = _vtxToTriAround[vertexIndex];
	for(unsigned int const& triIdx : triArround)
	{
		unsigned char& trisState = _trisState[triIdx];
		ASSERT(!TestStateFlags(trisState, TRI_STATE_PENDING_REMOVE));
		if(!TestStateFlags(trisState, TRI_STATE_HAS_TO_RECOMPUTE_NORMAL))
		{
			AddStateFlags(trisState, TRI_STATE_HAS_TO_RECOMPUTE_NORMAL);
			_trisIdxToRecomputeNormalOn.push_back(triIdx);
			unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
			for(int i = 0; i < 3; ++i)
			{
				unsigned int const& triVtxIdx = vtxsIdx[i];
				unsigned char& triVtxState = _vtxsState[triVtxIdx];
				ASSERT(!TestStateFlags(triVtxState, VTX_STATE_PENDING_REMOVE));
				if(!TestStateFlags(triVtxState, VTX_STATE_HAS_TO_RECOMPUTE_NORMAL))
				{
					AddStateFlags(triVtxState, VTX_STATE_HAS_TO_RECOMPUTE_NORMAL | VTX_STATE_HAS_TO_RECOMPUTE_SUB_MESH);
					_vtxsIdxToRecomputeNormalOn.push_back(triVtxIdx);
				}
			}
		}
	}
}

void Mesh::BuildOctree(BBox bbox)
{
#ifdef PROFILE_INFO
	printf("Start building octree...\n");
	clock_t begin = clock();
#endif // PROFILE_INFO

	{	// Build a square box (i.e a cube), with maximum extends regarding the input bbox given
		Vector3 extends = bbox.Extents();
		float maxExtends = max(max(extends.x, extends.y), extends.z);
		bbox.Encapsulate(Vector3(-maxExtends, -maxExtends, -maxExtends));
		bbox.Encapsulate(Vector3(maxExtends, maxExtends, maxExtends));
	}
	bbox.Scale(10.0f);

	std::vector<unsigned int> trianglesToInsert;
	trianglesToInsert.resize(_trisState.size());
	for(int i = 0; i < trianglesToInsert.size(); ++i)
		trianglesToInsert[i] = i;
	std::vector<unsigned int> verticesToInsert;
	verticesToInsert.resize(_vtxsState.size());
	for(int i = 0; i < verticesToInsert.size(); ++i)
		verticesToInsert[i] = i;
	_octreeRoot.reset(new OctreeCell(bbox, nullptr));
	_octreeRoot->Insert(*this, trianglesToInsert, verticesToInsert);
	RecomputeFragmentsBBox(true);

#ifdef PROFILE_INFO
	clock_t end = clock();
	printf("Octree built in %f\n", double(end - begin) / CLOCKS_PER_SEC);
#endif // PROFILE_INFO
}

void Mesh::WeldVertices(std::vector<unsigned int> const& triIn, std::vector<Vector3> const& vtxsIn, std::vector<unsigned int>& triOut, std::vector<Vector3>& vtxsOut)
{
#ifdef PROFILE_INFO
	printf("Start welding vertices\n");
	printf("\tInput vertex count %d\n", (int) vtxsIn.size());
	clock_t begin = clock();
#endif // PROFILE_INFO

	vtxsOut.clear();
	triOut.clear();
	vtxsOut.reserve(vtxsIn.size());
	triOut.reserve(triIn.size());

	std::vector<unsigned int> old2new(vtxsIn.size());
	std::map<Vector3, unsigned int> vertexToNewID;
	// Make new vertices
	for(int i = 0; i < vtxsIn.size(); ++i)
	{
		Vector3 const& vertex = vtxsIn[i];
		std::map<Vector3, unsigned int>::iterator itVtx = vertexToNewID.find(vertex);
		if(itVtx == vertexToNewID.end()) // Check to see if it's already been added
		{	// Add new vertex
			unsigned int vtxIndex = (unsigned int) vtxsOut.size();
			old2new[i] = vtxIndex;
			vertexToNewID[vertex] = vtxIndex;
			vtxsOut.push_back(vertex);
		}
		else // make duplicated vertex point to the new one
			old2new[i] = itVtx->second;
	}

	// Make new triangles
	for(int i = 0; i < triIn.size(); ++i)
		triOut.push_back(old2new[triIn[i]]);

	// Remove degenerated triangles 
	for(int i = 0; i < triOut.size();)
	{
		if((triOut[i] == triOut[i + 1])
			|| (triOut[i + 1] == triOut[i + 2])
			|| (triOut[i + 2] == triOut[i]))
		{
			triOut[i + 2] = triOut.back();
			triOut.pop_back();
			triOut[i + 1] = triOut.back();
			triOut.pop_back();
			triOut[i] = triOut.back();
			triOut.pop_back();
		}
		else
			i += 3;
	}

#ifdef PROFILE_INFO
	clock_t end = clock();
	printf("\tOutput vertex count %d\n", (int) vtxsOut.size());
	printf("\tWeld vertices time %f\n", double(end - begin) / CLOCKS_PER_SEC);
	printf("Vertices welded\n");
#endif // PROFILE_INFO
}

bool Mesh::IsManifold()
{
	return _IsManifold && !_IsOpen;
}

void Mesh::TagAndCollectOpenEdgesVertices(LoopBuilder *loopBuilder)
{
	_IsManifold = true;
	_IsOpen = false;
	ClearAllVerticesOnOpenEdge();
	for(unsigned int vtxIdx = 0; vtxIdx < _vertices.size(); ++vtxIdx)
	{
		AddStateFlags(_vtxsState[vtxIdx], VTX_STATE_ALREADY_TREATED);
		if(TestStateFlags(_vtxsState[vtxIdx], VTX_STATE_PENDING_REMOVE) == false)
		{
			for(unsigned int const& triIdx : _vtxToTriAround[vtxIdx])
			{
				ASSERT(triIdx < _trisState.size());
				if(TestStateFlags(_trisState[triIdx], TRI_STATE_PENDING_REMOVE))
					continue;
				unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
				for(int i = 0; i < 3; ++i)
				{
					unsigned int otherVtxIdx = vtxsIdx[i];
					if((otherVtxIdx != vtxIdx) && !TestStateFlags(_vtxsState[otherVtxIdx], VTX_STATE_ALREADY_TREATED))
					{	// Test that an edge is only shared by two triangles
						std::vector<unsigned int> const& triAroundA = _vtxToTriAround[vtxIdx];
						std::vector<unsigned int> const& triAroundB = _vtxToTriAround[otherVtxIdx];
						unsigned int nbSharedTriangles = 0;
						for(unsigned int triIdxA : triAroundA)		// Todo : O(n), of course optimize this
						{
							for(unsigned int triIdxB : triAroundB)
							{
								if(triIdxA == triIdxB)
								{	// Triangle common to vertex A and B
									if(TestStateFlags(_trisState[triIdxA], TRI_STATE_PENDING_REMOVE) == false)
										++nbSharedTriangles;
									break;
								}
							}
						}
						if(nbSharedTriangles == 1)
						{
							_IsOpen = true;
							SetVertexOnOpenEdge(vtxIdx);
							SetVertexOnOpenEdge(otherVtxIdx);
							if(loopBuilder)
								loopBuilder->RegisterEdge(vtxIdx, otherVtxIdx);
						}
						else if(nbSharedTriangles > 2)
							_IsManifold = false;
					}
				}
			}
		}
	}
	ClearAllVerticesAlreadyTreated();
}

#ifdef MESH_CONSISTENCY_CHECK
void Mesh::CheckTriangleIsCorrect(unsigned int triIdx, bool testFlateness)
{
	auto TestEdge = [&](unsigned int a, unsigned int b)
	{	// Test that an edge is only shared by two triangles
		std::vector<unsigned int> const& triAroundA = _vtxToTriAround[a];
		std::vector<unsigned int> const& triAroundB = _vtxToTriAround[b];
		unsigned int nbSharedTriangles = 0;
		for(unsigned int triIdxA : triAroundA)		// Todo : O(n), of course optimize this
		{
			for(unsigned int triIdxB : triAroundB)
			{
				if(triIdxA == triIdxB)
				{	// triangle common to vertex A and B
					if(TestStateFlags(_trisState[triIdxA], TRI_STATE_PENDING_REMOVE) == false)
						++nbSharedTriangles;
					break;
				}
			}
		}
		ASSERT((nbSharedTriangles == 2) || ((nbSharedTriangles == 1) && IsVertexOnOpenEdge(a) && IsVertexOnOpenEdge(b)));	//  Should be always the case on a manifold mesh
	};
	if(!consistencyCheckOn)
		return;
	if(IsTriangleToBeRemoved(triIdx))
		return;
	unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
	unsigned int vtxId0 = vtxsIdx[0];
	unsigned int vtxId1 = vtxsIdx[1];
	unsigned int vtxId2 = vtxsIdx[2];
	ASSERT(!IsVertexToBeRemoved(vtxId0));
	ASSERT(!IsVertexToBeRemoved(vtxId1));
	ASSERT(!IsVertexToBeRemoved(vtxId2));
	if(testFlateness)
	{
		Vector3 const& vtx0 = _vertices[vtxId0];
		Vector3 const& vtx1 = _vertices[vtxId1];
		Vector3 const& vtx2 = _vertices[vtxId2];
		float squareLengthEdge1 = (vtx0 - vtx1).LengthSquared();
		float squareLengthEdge2 = (vtx1 - vtx2).LengthSquared();
		float squareLengthEdge3 = (vtx2 - vtx0).LengthSquared();
		ASSERT(squareLengthEdge1 != 0.0f);
		ASSERT(squareLengthEdge2 != 0.0f);
		ASSERT(squareLengthEdge3 != 0.0f);
	}
	TestEdge(vtxId0, vtxId1);
	TestEdge(vtxId1, vtxId2);
	TestEdge(vtxId2, vtxId0);
}

void Mesh::CheckVertexIsCorrect(unsigned int vtxIdx)
{
	if(!consistencyCheckOn)
		return;
	ASSERT(TestStateFlags(_vtxsState[vtxIdx], VTX_STATE_PENDING_REMOVE)
		|| (_vtxToTriAround[vtxIdx].size() >= 3)
		|| ((_vtxToTriAround[vtxIdx].size() >= 1) && IsVertexOnOpenEdge(vtxIdx)));		// Must be the case on a manifold mesh
	Vector3 refFaceNormal;
	bool hasRefFaceNormal = false;
	for(unsigned int const& triIdx : _vtxToTriAround[vtxIdx])
	{
		ASSERT(triIdx < _trisState.size());
		if(TestStateFlags(_trisState[triIdx], TRI_STATE_PENDING_REMOVE))
			continue;
		unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
		// *** edge shared by 2 triangles only check ****
#if 1
		bool found = false;
		for(int i = 0; i < 3; ++i)
		{
			if(vtxsIdx[i] == vtxIdx)
				found = true;
			else
			{	// Test that an edge is only shared by two triangles
				std::vector<unsigned int> const& triAroundA = _vtxToTriAround[vtxIdx];
				std::vector<unsigned int> const& triAroundB = _vtxToTriAround[vtxsIdx[i]];
				unsigned int nbSharedTriangles = 0;
				for(unsigned int triIdxA : triAroundA)		// Todo : O(n), of course optimize this
				{
					for(unsigned int triIdxB : triAroundB)
					{
						if(triIdxA == triIdxB)
						{	// triangle common to vertex A and B
							if(TestStateFlags(_trisState[triIdxA], TRI_STATE_PENDING_REMOVE) == false)
								++nbSharedTriangles;
							break;
						}
					}
				}
				ASSERT((nbSharedTriangles == 2) || ((nbSharedTriangles == 1) && IsVertexOnOpenEdge(vtxIdx) && IsVertexOnOpenEdge(vtxsIdx[i])));	//  Should be always the case on a manifold mesh
			}
		}
		ASSERT(found);
#endif
		// *** flipped triangle check ****
		Vector3& vtx1 = _vertices[vtxsIdx[0]];
		Vector3& vtx2 = _vertices[vtxsIdx[1]];
		Vector3& vtx3 = _vertices[vtxsIdx[2]];
		Vector3 p1p2 = vtx2 - vtx1;
		Vector3 p1p3 = vtx3 - vtx1;
		Vector3 triNormal = p1p2.Cross(p1p3);
		float triNormalLength = triNormal.Length();
		if(triNormalLength != 0.0f)
		{
			triNormal /= triNormalLength;
			if(!hasRefFaceNormal)
			{	// Get the first face normal
				refFaceNormal = triNormal;
				hasRefFaceNormal = true;
			}
			else
			{	// Check if the angle is not too much, if so, this really might be a flipped face
#ifndef THICKNESS_HANDLER_WIP
				//float dot = refFaceNormal.Dot(triNormal);
				//ASSERT(dot > cosf(179.0f * float(M_PI) / 180.0f));
#endif // !THICKNESS_HANDLER_WIP
			}
		}
		// Check that triangle to vertex relationship is set in both ways
		bool bFound = false;
		for(unsigned int i = 0; i < 3; ++i)
		{
			if(vtxsIdx[i] == vtxIdx)
			{
				bFound = true;
				break;
			}
		}
		ASSERT(bFound);
	}
}

void Mesh::CheckMeshIsCorrect()
{
	if(!consistencyCheckOn)
		return;
#ifdef THICKNESS_HANDLER_WIP
	/*for(unsigned int i = 0; i < _trisState.size(); ++i)
		CheckTriangleIsCorrect(i, false);*/
#endif // THICKNESS_HANDLER_WIP
	for(unsigned int i = 0; i < _vertices.size(); ++i)
		CheckVertexIsCorrect(i);
}
#endif // MESH_CONSISTENCY_CHECK

bool Mesh::CanUndo()
{
	return _snapshotNextPos > 1;
}

bool Mesh::Undo()
{
	if(_snapshotNextPos > 1)
	{
		--_snapshotNextPos;
		RestoreMeshToCurSnapshot();
		return true;
	}
	return false;
}

bool Mesh::CanRedo()
{
	return _snapshotNextPos < (unsigned int) _verticesSnapshots.size();
}

bool Mesh::Redo()
{
	if(_snapshotNextPos < (unsigned int) _verticesSnapshots.size())
	{
		++_snapshotNextPos;
		RestoreMeshToCurSnapshot();
		return true;
	}
	return false;
}

void Mesh::TakeSnapShot()
{
	if(_snapshotNextPos > MAXIMUM_UNDO_COUNT)
	{	// Limit the maximum snapshot stored, to avoid using too much memory
		_verticesSnapshots.pop_front();
		_trianglesSnapshots.pop_front();
		--_snapshotNextPos;
	}
	if(_snapshotNextPos != (unsigned int) _verticesSnapshots.size())
	{	// Erase newest snapshot if we have already roll back to older revisions
		_verticesSnapshots.resize(_snapshotNextPos);
		_trianglesSnapshots.resize(_snapshotNextPos);
	}
	_verticesSnapshots.push_back(_vertices);
	_trianglesSnapshots.push_back(_triangles);
	++_snapshotNextPos;
}

void Mesh::RestoreMeshToCurSnapshot()
{
	_vertices = _verticesSnapshots[_snapshotNextPos - 1];
	_triangles = _trianglesSnapshots[_snapshotNextPos - 1];
	RebuildMeshData(false, false, false);
}

void Mesh::Transform(Matrix3 const& rotAndScale, Vector3 const& position)
{
	for(Vector3& vtx : _vertices)
	{
		rotAndScale.Transform(vtx);
		vtx += position;
	}
	for(Vector3& vtxNrm : _vtxsNormal)
		rotAndScale.Transform(vtxNrm);
	for(Vector3& triNrm : _trisNormal)
		rotAndScale.Transform(triNrm);
	for(BSphere& triBSphere : _trisBSphere)
		triBSphere.Transform(rotAndScale, position);
	if(_octreeRoot != nullptr)
		_octreeRoot.reset(nullptr);	// Clear old octree
	// Compute bbox
	BBox bbox;
	for(Vector3 const& vertex : _vertices)
		bbox.Encapsulate(vertex);
	// Build new octree
	if(bbox.IsValid())
		BuildOctree(bbox);
}

#include "GenBox.h"
#include "GenCylinder.h"
#include "GenSphere.h"
#include "MeshLoader.h"
bool Mesh::CSGTest()
{
	// Gen other obj
#if 1
	BBox const& bbox = GetBBox();
	float modelRadius = bbox.Size().Length() * (0.5f / (float) M_SQRT2);
	//GenBox otherObjGenerator;
	//std::unique_ptr<Mesh> otherObj(otherObjGenerator.Generate(300.0f, 60.0f, 60.0f));
	GenCylinder otherObjGenerator;
	std::unique_ptr<Mesh> otherObj(otherObjGenerator.Generate(modelRadius * 2.5f, modelRadius / 3.0f));
#else
	//GenSphere otherObjGenerator;
	//std::unique_ptr<Mesh> otherObj(otherObjGenerator.Generate(100.0f));
	MeshLoader meshLoader;
	std::unique_ptr<Mesh> otherObj(meshLoader.LoadFromFile("C:\\data\\From STL Finder\\gearknob.tct"));
#endif
	float mxtContent[9] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, -1.f, 0.f };	// 90 degrees around x axis
	Matrix3 otherObjMtx(mxtContent);
	Vector3 otherObjPos(modelRadius * 0.2f, 0.f, 0.f);
	return CSGSubtract(*otherObj, otherObjMtx, otherObjPos, false);
}

bool Mesh::CSGMerge(Mesh& otherMesh, Matrix3 const& rotAndScale, Vector3 const& position, bool recenterResult)
{
#ifdef PROFILE_INFO
	printf("Start CSG operation...\n");
	clock_t begin = clock();
#endif // PROFILE_INFO
	if((_octreeRoot == nullptr) || (otherMesh._octreeRoot == nullptr))
		return false;
	// Clone othermesh because CSG/transform operation will spoil it a bit
	Mesh otherMeshCopy(otherMesh, false, false);
	// Positioning of the other mesh
	otherMeshCopy.Transform(rotAndScale, position);
	// CSG
	Csg csg(*this, otherMeshCopy, *this);
	bool result = csg.ComputeCSGOperation(Csg::MERGE, recenterResult);
#ifdef PROFILE_INFO
	clock_t end = clock();
	printf("CSG operation done in %f\n", double(end - begin) / CLOCKS_PER_SEC);
#endif // PROFILE_INFO
	return result;
}

bool Mesh::CSGSubtract(Mesh& otherMesh, Matrix3 const& rotAndScale, Vector3 const& position, bool recenterResult)
{
#ifdef PROFILE_INFO
	printf("Start CSG operation...\n");
	clock_t begin = clock();
#endif // PROFILE_INFO
	if((_octreeRoot == nullptr) || (otherMesh._octreeRoot == nullptr))
		return false;
	// Clone othermesh because CSG/transform operation will spoil it a bit
	Mesh otherMeshCopy(otherMesh, false, false);
	// Positioning of the other mesh
	otherMeshCopy.Transform(rotAndScale, position);
	// CSG
	Csg csg(*this, otherMeshCopy, *this);
	bool result = csg.ComputeCSGOperation(Csg::SUBTRACT, recenterResult);
#ifdef PROFILE_INFO
	clock_t end = clock();
	printf("CSG operation done in %f\n", double(end - begin) / CLOCKS_PER_SEC);
#endif // PROFILE_INFO
	return result;
}

bool Mesh::CSGIntersect(Mesh& otherMesh, Matrix3 const& rotAndScale, Vector3 const& position, bool recenterResult)
{
#ifdef PROFILE_INFO
	printf("Start CSG operation...\n");
	clock_t begin = clock();
#endif // PROFILE_INFO
	if((_octreeRoot == nullptr) || (otherMesh._octreeRoot == nullptr))
		return false;
	// Clone othermesh because CSG/transform operation will spoil it a bit
	Mesh otherMeshCopy(otherMesh, false, false);
	// Positioning of the other mesh
	otherMeshCopy.Transform(rotAndScale, position);
	// CSG
	Csg csg(*this, otherMeshCopy, *this);
	bool result = csg.ComputeCSGOperation(Csg::INTERSECT, recenterResult);
#ifdef PROFILE_INFO
	clock_t end = clock();
	printf("CSG operation done in %f\n", double(end - begin) / CLOCKS_PER_SEC);
#endif // PROFILE_INFO
	return result;
}

bool Mesh::RemoveFlatTriangle(unsigned int triIdx)
{
	if(IsTriangleToBeRemoved(triIdx))
		return false;	// Don't treat triangles that are to be removed
#ifdef MESH_CONSISTENCY_CHECK
	CheckTriangleIsCorrect(triIdx, false);
#endif // MESH_CONSISTENCY_CHECK
	unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
	unsigned int vtxId0 = vtxsIdx[0];
	unsigned int vtxId1 = vtxsIdx[1];
	unsigned int vtxId2 = vtxsIdx[2];
	Vector3 const& vtx0 = _vertices[vtxId0];
	Vector3 const& vtx1 = _vertices[vtxId1];
	Vector3 const& vtx2 = _vertices[vtxId2];
	// will find the tallest edge
	float squaredLengthEdge1 = (vtx0 - vtx1).LengthSquared();
	float squaredLengthEdge2 = (vtx1 - vtx2).LengthSquared();
	float squaredLengthEdge3 = (vtx2 - vtx0).LengthSquared();
	float minSquaredLength = min(min(squaredLengthEdge1, squaredLengthEdge2), squaredLengthEdge3);
	float maxSquaredLength = max(max(squaredLengthEdge1, squaredLengthEdge2), squaredLengthEdge3);
	if(maxSquaredLength == 0.0f)
	{	// All three points of the triangle have the same coordinate
		DeleteTriangle(triIdx);
		// Merge all three vertices into one (will merge point index 1 and 2 to the 0 index one)
		DeleteVertexAndReconnectTrianglesToAnotherOne(vtxId2, vtxId0);
		DeleteVertexAndReconnectTrianglesToAnotherOne(vtxId1, vtxId0);
		return true;
	}
	else if(minSquaredLength == 0.0f)
	{	// The triangle is flat, it's like a line
		DeleteTriangle(triIdx);
		// Merge the two vertices of the zero length edge
		if(squaredLengthEdge1 == 0.0f)
			DeleteVertexAndReconnectTrianglesToAnotherOne(vtxId1, vtxId0);
		else if(squaredLengthEdge2 == 0.0f)
			DeleteVertexAndReconnectTrianglesToAnotherOne(vtxId2, vtxId1);
		else //if(squaredLengthEdge3 == 0.0f)
			DeleteVertexAndReconnectTrianglesToAnotherOne(vtxId0, vtxId2);
		return true;
	}
	return false;
}

void Mesh::DeleteTriangle(unsigned int triIdx)
{
	ASSERT(!IsTriangleToBeRemoved(triIdx));
	if(IsTriangleToBeRemoved(triIdx) == false)
	{
		// Update "tri around vertex" data to prevent accessing to those triangles while processing next ones for deletion
		unsigned int const* removedTriVtxsIdx = &(_triangles[triIdx * 3]);
		for(int i = 0; i < 3; ++i)
		{
			unsigned int const& vtxToModify = removedTriVtxsIdx[i];
			if(GetTrianglesAroundVertex(vtxToModify).size() == 1)
				SetVertexToBeRemoved(vtxToModify);
			RemoveTriangleAroundVertex(vtxToModify, triIdx);
		}
		// Remove triangle
		SetTriangleToBeRemoved(triIdx);
		// Make the triangle fits in a single point
		_triangles[triIdx * 3 + 1] = _triangles[triIdx * 3];
		_triangles[triIdx * 3 + 2] = _triangles[triIdx * 3];
	}
}

void Mesh::DeleteVertexAndReconnectTrianglesToAnotherOne(unsigned int vtxIdxToDelete, unsigned int vtxIdxToReconnectTo)
{
	if(vtxIdxToDelete != vtxIdxToReconnectTo)
	{
		ASSERT(IsVertexToBeRemoved(vtxIdxToReconnectTo) == false);
		// Loop on all the triangles connected to that vertex, and reconnect to the other vertex
		for(unsigned int i = 0; i < _vtxToTriAround[vtxIdxToDelete].size();)
		{
			unsigned int triIdx = _vtxToTriAround[vtxIdxToDelete][i];
			if(!IsTriangleToBeRemoved(triIdx))
			{
				unsigned int* triVtxsIdx = &(_triangles[triIdx * 3]);
				bool deleteTriangle = false;
				for(int j = 0; j < 3; ++j)
				{
					if(triVtxsIdx[j] == vtxIdxToReconnectTo)
					{	// If we would continue, the triangle will have two vertices at "vtxIdxToReconnectTo". Happen to the triangle sharing edge with a triangle we are deleting. In that case, delete the flat triangle
						deleteTriangle = true;
						break;
					}
				}
				if(deleteTriangle)
				{
					DeleteTriangle(triIdx);
					continue;
				}
				for(int j = 0; j < 3; ++j)
				{
					if(triVtxsIdx[j] == vtxIdxToDelete)
					{
						triVtxsIdx[j] = vtxIdxToReconnectTo;
						AddTriangleAroundVertex(vtxIdxToReconnectTo, triIdx);
						++i;
						break;
					}
				}
			}
			else
				++i;
		}
		// Now we can set the vertex to be deleted
		ClearTriangleAroundVertex(vtxIdxToDelete);
		SetVertexToBeRemoved(vtxIdxToDelete);
		SetHasToRecomputeNormal(vtxIdxToReconnectTo);
	}
}

void Mesh::DetectAndTreatDegeneratedEdgeAroundVertex(unsigned int vtxToTestIdx, std::vector<unsigned int>& retNewVerticesIdx)
{
	auto EmergencyCleanup = [&](unsigned int degenEdgeVtx1, unsigned int degenEdgeVtx2)
	{
		// In most casesIt's a knot, making like an 8 as shape.
		// Solution for the moment: collapse the edge to one single vertex
		Vector3 a = _vertices[degenEdgeVtx1];
		Vector3 b = _vertices[degenEdgeVtx2];
		Vector3 delta = b - a;
		// Get affected triangles, this will be a merge between triangles around "a" and "b"
		std::vector<unsigned int> const& triAroundA = GetTrianglesAroundVertex(degenEdgeVtx1);
		std::vector<unsigned int> const& triAroundB = GetTrianglesAroundVertex(degenEdgeVtx2);
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
		ASSERT(triToRemove.size() == 4);
		// Replace "a" vertex with the middle coordinate
		_vertices[degenEdgeVtx1] = delta * 0.5f + a;
		// For each triangle that was around "b", relink to "a" vertex
		for(unsigned int triIdxB : triAroundB)
		{
			bool hasToRemove = false;
			for(unsigned int triIdxToRemove : triToRemove)
			{
				if(triIdxB == triIdxToRemove)
					hasToRemove = true;
			}
			if(!hasToRemove)
			{
				unsigned int* vtxsIdx = &(_triangles[triIdxB * 3]);
				unsigned int i = 0;
				for(; i < 3; ++i)
				{
					if(vtxsIdx[i] == degenEdgeVtx2)
					{
						vtxsIdx[i] = degenEdgeVtx1;
						AddTriangleAroundVertex(degenEdgeVtx1, triIdxB);
						break;
					}
				}
				ASSERT(i < 3);	// We should have find something
			}
		}
		ClearTriangleAroundVertex(degenEdgeVtx2);
		// Register "b" vertex for removal
		SetVertexToBeRemoved(degenEdgeVtx2);
		// Register triToRemove for removal
		for(unsigned int triIdxToRemove : triToRemove)
			DeleteTriangle(triIdxToRemove);
		// Remove degenerated triangles
		DetectAndRemoveDegeneratedTrianglesAroundVertex(degenEdgeVtx1, retNewVerticesIdx);
		// Set recompute normal request
		if(IsVertexToBeRemoved(degenEdgeVtx1) == false)	// The vertex could be tagged to be removed: This happens if all resulting edges and triangles were degenerated.
			SetHasToRecomputeNormal(degenEdgeVtx1);
	};
	std::vector<unsigned int> const& triAround = GetTrianglesAroundVertex(vtxToTestIdx);
	unsigned int newVerticesBefore = (unsigned int) retNewVerticesIdx.size();
	// Try to find degenerated edge: an edge that link more than two triangles.
	std::map<unsigned int, unsigned int> triVerticesOccurences;
	std::set<unsigned int> otherWeldVertices;
	for(unsigned int const& triIdx : triAround)
	{
		unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
		for(int i = 0; i < 3; ++i)
		{
			unsigned int const& vtxIdx = vtxsIdx[i];
			if(vtxIdx != vtxToTestIdx)
			{
				if(triVerticesOccurences.find(vtxIdx) == triVerticesOccurences.end())
					triVerticesOccurences[vtxIdx] = 1;
				else
				{
					++triVerticesOccurences[vtxIdx];
					if(triVerticesOccurences[vtxIdx] > 2)
						otherWeldVertices.insert(vtxIdx);
				}
			}
		}
	}

#ifdef RETESS_DEBUGGING
	static int count = 0;
	count++;
#ifdef _DEBUG
	if(0)//count == 182137)
	{
		consistencyCheckOn = true;
		__debugbreak();
		if(!otherWeldVertices.empty())
		{
			unsigned int debug_degenEdgeVtx1 = vtxToTestIdx;
			unsigned int debug_degenEdgeVtx2 = *(otherWeldVertices.begin());
			std::vector<unsigned int> vtxIdxToDebugAroundBefore;
			vtxIdxToDebugAroundBefore.push_back(debug_degenEdgeVtx1);
			SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "degenVtx1.obj");
			vtxIdxToDebugAroundBefore.clear();
			vtxIdxToDebugAroundBefore.push_back(debug_degenEdgeVtx2);
			SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "degenVtx2.obj");
			vtxIdxToDebugAroundBefore.clear();
			vtxIdxToDebugAroundBefore.push_back(debug_degenEdgeVtx1);
			vtxIdxToDebugAroundBefore.push_back(debug_degenEdgeVtx2);
			SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "degenBothVtx.obj");
		}
	}
#endif // _DEBUG
#endif // RETESS_DEBUGGING
	while(!otherWeldVertices.empty() && !triAround.empty())
	{
		unsigned int degenEdgeVtx1 = vtxToTestIdx;
		unsigned int degenEdgeVtx2 = *(otherWeldVertices.begin());
		// Find the triangles that share that degenerated edge
		std::set<unsigned int> triAroundTouchingEdge;
		for(unsigned int triIdx : triAround)
		{
			unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
			unsigned int nbVtxMatching = 0;
			for(int i = 0; i < 3; ++i)
			{
				unsigned int const& vtxIdx = vtxsIdx[i];
				unsigned int matchingIndex[2];
				if((vtxIdx == degenEdgeVtx1) || (vtxIdx == degenEdgeVtx2))
				{
					matchingIndex[nbVtxMatching] = i;
					++nbVtxMatching;
				}
				if(nbVtxMatching == 2)
				{
					triAroundTouchingEdge.insert(triIdx);
					break;
				}
			}
		}
		ASSERT(triAroundTouchingEdge.size() == 4);
		if(triAroundTouchingEdge.size() != 4)
		{
			EmergencyCleanup(degenEdgeVtx1, degenEdgeVtx2);
			return;
		}
#if 1
		// We take the first triangle touching edge, and we will run through connectivity to find all the touching triangles on its side
		unsigned int refStartTriangle = *(triAroundTouchingEdge.begin());
		unsigned int newVtx1Id = UNDEFINED_NEW_ID;
		unsigned int newVtx2Id = UNDEFINED_NEW_ID;
		std::set<unsigned int> collectedTriAroundDegenVtx1;
		std::set<unsigned int> collectedTriAroundDegenVtx2;
		{
			std::vector<unsigned int> triToTag;
			// Iterate around first edge vertex
			triToTag.push_back(refStartTriangle);			
			collectedTriAroundDegenVtx1.insert(triToTag[0]);
			triAroundTouchingEdge.erase(triToTag[0]);
			while(!triToTag.empty())
			{
				// Collect one triangle loop touching a degenerated vertex from the edge
				unsigned int const* vtxsTagIdx = &(_triangles[triToTag.back() * 3]);
				triToTag.pop_back();
				for(int i = 0; i < 3; ++i)
				{
					if((vtxsTagIdx[i] != degenEdgeVtx1) && (vtxsTagIdx[i] != degenEdgeVtx2))	// Vertex that doesn't belong to the degenerated edge
					{
						// Run through triangles around the vertex 
						std::vector<unsigned int> const& triTagAround = GetTrianglesAroundVertex(vtxsTagIdx[i]);
						for(unsigned int triTagAroundIdx : triTagAround)
						{
							// If the triangle has one of its vertex that belong to the degenrated edge, we can select it
							unsigned int const* vtxsTagAroundIdx = &(_triangles[triTagAroundIdx * 3]);
							for(int j = 0; j < 3; ++j)
							{
								if(vtxsTagAroundIdx[j] == degenEdgeVtx1)
								{
									if(collectedTriAroundDegenVtx1.find(triTagAroundIdx) == collectedTriAroundDegenVtx1.end())
									{
										triToTag.push_back(triTagAroundIdx);
										collectedTriAroundDegenVtx1.insert(triTagAroundIdx);
										if(triAroundTouchingEdge.find(triTagAroundIdx) != triAroundTouchingEdge.end())
											triAroundTouchingEdge.erase(triTagAroundIdx);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		{
			std::vector<unsigned int> triToTag;
			// Iterate around first edge vertex
			triToTag.push_back(refStartTriangle);
			collectedTriAroundDegenVtx2.insert(triToTag[0]);
			triAroundTouchingEdge.erase(triToTag[0]);
			while(!triToTag.empty())
			{
				// Collect one triangle loop touching a degenerated vertex from the edge
				unsigned int const* vtxsTagIdx = &(_triangles[triToTag.back() * 3]);
				triToTag.pop_back();
				for(int i = 0; i < 3; ++i)
				{
					if((vtxsTagIdx[i] != degenEdgeVtx1) && (vtxsTagIdx[i] != degenEdgeVtx2))	// Vertex that doesn't belong to the degenerated edge
					{
						// Run through triangles around the vertex 
						std::vector<unsigned int> const& triTagAround = GetTrianglesAroundVertex(vtxsTagIdx[i]);
						for(unsigned int triTagAroundIdx : triTagAround)
						{
							// If the triangle has one of its vertex that belong to the degenrated edge, we can select it
							unsigned int const* vtxsTagAroundIdx = &(_triangles[triTagAroundIdx * 3]);
							for(int j = 0; j < 3; ++j)
							{
								if(vtxsTagAroundIdx[j] == degenEdgeVtx2)
								{
									if(collectedTriAroundDegenVtx2.find(triTagAroundIdx) == collectedTriAroundDegenVtx2.end())
									{
										triToTag.push_back(triTagAroundIdx);
										collectedTriAroundDegenVtx2.insert(triTagAroundIdx);
										if(triAroundTouchingEdge.find(triTagAroundIdx) != triAroundTouchingEdge.end())
											triAroundTouchingEdge.erase(triTagAroundIdx);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		if(triAroundTouchingEdge.size() == 2)
		{
			{
				// Unlink colected triangles from that vertex
				for(unsigned int triIdx : collectedTriAroundDegenVtx1)
				{
					unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
					for(int i = 0; i < 3; ++i)
					{
						if(vtxsIdx[i] == degenEdgeVtx1)
						{
							RemoveTriangleAroundVertex(degenEdgeVtx1, triIdx);
							break;
						}
					}
				}
				// Create one new vertex, at the same position of the degenerated vertex
				Vector3 newVtx(_vertices[degenEdgeVtx1]);
				newVtx1Id = AddVertex(newVtx);
				retNewVerticesIdx.push_back(newVtx1Id);
				// Replace vertex index on the collected triangles
				for(unsigned int triIdx : collectedTriAroundDegenVtx1)
				{
					unsigned int* vtxsIdx = &(_triangles[triIdx * 3]);
					for(int i = 0; i < 3; ++i)
					{
						unsigned int& vtxIdx = vtxsIdx[i];
						if(vtxIdx == degenEdgeVtx1)
						{
							vtxIdx = newVtx1Id;
							AddTriangleAroundVertex(newVtx1Id, triIdx);
						}
					}
				}
			}
			{
				// Unlink colected triangles from that vertex
				for(unsigned int triIdx : collectedTriAroundDegenVtx2)
				{
					unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
					for(int i = 0; i < 3; ++i)
					{
						if(vtxsIdx[i] == degenEdgeVtx2)
						{
							RemoveTriangleAroundVertex(degenEdgeVtx2, triIdx);
							break;
						}
					}
				}
				// Create one new vertex, at the same position of the degenerated vertex
				Vector3 newVtx(_vertices[degenEdgeVtx2]);
				newVtx2Id = AddVertex(newVtx);
				retNewVerticesIdx.push_back(newVtx2Id);
				// Replace vertex index on the collected triangles
				for(unsigned int triIdx : collectedTriAroundDegenVtx2)
				{
					unsigned int* vtxsIdx = &(_triangles[triIdx * 3]);
					for(int i = 0; i < 3; ++i)
					{
						unsigned int& vtxIdx = vtxsIdx[i];
						if(vtxIdx == degenEdgeVtx2)
						{
							vtxIdx = newVtx2Id;
							AddTriangleAroundVertex(newVtx2Id, triIdx);
						}
					}
				}
			}
			ASSERT(triAroundTouchingEdge.size() == 2);
			if(triAroundTouchingEdge.size() != 2)
				return;
			if((newVtx1Id == UNDEFINED_NEW_ID) || (newVtx2Id == UNDEFINED_NEW_ID))
				return;
			// Remove degenerated quads and triangles
#ifdef RETESS_DEBUGGING
#ifdef _DEBUG
			if(0)//count == 182137)
			{
				__debugbreak();
				if(!otherWeldVertices.empty())
				{
					std::vector<unsigned int> vtxIdxToDebugAroundBefore;
					vtxIdxToDebugAroundBefore.push_back(degenEdgeVtx1);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "degenVtx1-2.obj");
					vtxIdxToDebugAroundBefore.clear();
					vtxIdxToDebugAroundBefore.push_back(degenEdgeVtx2);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "degenVtx2-2.obj");
					vtxIdxToDebugAroundBefore.clear();
					vtxIdxToDebugAroundBefore.push_back(degenEdgeVtx1);
					vtxIdxToDebugAroundBefore.push_back(degenEdgeVtx2);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "degenBothVtx-2.obj");
					// ...
					vtxIdxToDebugAroundBefore.clear();
					vtxIdxToDebugAroundBefore.push_back(newVtx1Id);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "newVtx1-2.obj");
					vtxIdxToDebugAroundBefore.clear();
					vtxIdxToDebugAroundBefore.push_back(newVtx2Id);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "newVtx2-2.obj");
					vtxIdxToDebugAroundBefore.clear();
					vtxIdxToDebugAroundBefore.push_back(newVtx1Id);
					vtxIdxToDebugAroundBefore.push_back(newVtx2Id);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "newBothVtx-2.obj");
				}
			}
#endif // _DEBUG
#endif // RETESS_DEBUGGING
			DetectAndRemoveDegeneratedQuadsAroundEdge(degenEdgeVtx1, degenEdgeVtx2);
			DetectAndRemoveDegeneratedQuadsAroundEdge(newVtx1Id, newVtx2Id);
			DetectAndRemoveDegeneratedTrianglesAroundVertex(degenEdgeVtx1, retNewVerticesIdx);
			DetectAndRemoveDegeneratedTrianglesAroundVertex(degenEdgeVtx2, retNewVerticesIdx);
			DetectAndRemoveDegeneratedTrianglesAroundVertex(newVtx1Id, retNewVerticesIdx);
			DetectAndRemoveDegeneratedTrianglesAroundVertex(newVtx2Id, retNewVerticesIdx);
#ifdef RETESS_DEBUGGING
#ifdef _DEBUG
			if(0)//count == 182137)
			{
				__debugbreak();
				if(!otherWeldVertices.empty())
				{
					std::vector<unsigned int> vtxIdxToDebugAroundBefore;
					vtxIdxToDebugAroundBefore.push_back(degenEdgeVtx1);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "degenVtx1-3.obj");
					vtxIdxToDebugAroundBefore.clear();
					vtxIdxToDebugAroundBefore.push_back(degenEdgeVtx2);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "degenVtx2-3.obj");
					vtxIdxToDebugAroundBefore.clear();
					vtxIdxToDebugAroundBefore.push_back(degenEdgeVtx1);
					vtxIdxToDebugAroundBefore.push_back(degenEdgeVtx2);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "degenBothVtx-3.obj");
					// ...
					vtxIdxToDebugAroundBefore.clear();
					vtxIdxToDebugAroundBefore.push_back(newVtx1Id);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "newVtx1-3.obj");
					vtxIdxToDebugAroundBefore.clear();
					vtxIdxToDebugAroundBefore.push_back(newVtx2Id);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "newVtx2-3.obj");
					vtxIdxToDebugAroundBefore.clear();
					vtxIdxToDebugAroundBefore.push_back(newVtx1Id);
					vtxIdxToDebugAroundBefore.push_back(newVtx2Id);
					SaveMeshPieceForVisualDebugGivingVertices(vtxIdxToDebugAroundBefore, "newBothVtx-3.obj");
				}
			}
#endif // _DEBUG
#endif // RETESS_DEBUGGING
			// Prepare for next loop if there is
			otherWeldVertices.erase(degenEdgeVtx2);
			if(!otherWeldVertices.empty() && !triAround.empty())
			{	// Does the remaining otherWeldVertices has been moved to the other side of the split zone ?
				degenEdgeVtx2 = *(otherWeldVertices.begin());
				std::vector<unsigned int> testTriAroundTouchingEdge;
				for(unsigned int triIdx : triAround)
				{
					unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
					unsigned int nbVtxMatching = 0;
					for(int i = 0; i < 3; ++i)
					{
						unsigned int const& vtxIdx = vtxsIdx[i];
						unsigned int matchingIndex[2];
						if((vtxIdx == degenEdgeVtx1) || (vtxIdx == degenEdgeVtx2))
						{
							matchingIndex[nbVtxMatching] = i;
							++nbVtxMatching;
						}
						if(nbVtxMatching == 2)
						{
							testTriAroundTouchingEdge.push_back(triIdx);
							break;
						}
					}
				}
				if(testTriAroundTouchingEdge.empty())
					vtxToTestIdx = newVtx1Id;	// Was moved to the other side of the split zone
			}
		}
		else
		{
			EmergencyCleanup(degenEdgeVtx1, degenEdgeVtx2);
			return;
		}
#else
		// We take the first triangle touching edge, and we will run through connectivity to find all the touching triangles on its side
		unsigned int refStartTriangle = *(triAroundTouchingEdge.begin());
		std::vector<unsigned int> triToTag;
		// Iterate around first edge vertex
		triToTag.push_back(refStartTriangle);
		std::set<unsigned int> triOnOneSideOfDegenEdge;
		triOnOneSideOfDegenEdge.insert(triToTag[0]);
		std::set<unsigned int> collectedTri;
		collectedTri.insert(triToTag[0]);
		triAroundTouchingEdge.erase(triToTag[0]);
		while(!triToTag.empty())
		{
			// Iterate over the triangles vertices
			unsigned int const* vtxsTagIdx = &(_triangles[triToTag.back() * 3]);
			triToTag.pop_back();
			for(int i = 0; i < 3; ++i)
			{
				if((vtxsTagIdx[i] != degenEdgeVtx1) && (vtxsTagIdx[i] != degenEdgeVtx2))	// Vertex that doesn't belong to the degenerated edge
				{
					// Run through triangles around the vertex 
					std::vector<unsigned int> const& triTagAround = GetTrianglesAroundVertex(vtxsTagIdx[i]);
					for(unsigned int triTagAroundIdx : triTagAround)
					{
						// If the triangle has one of its vertex that belong to the degenrated edge, we can select it
						unsigned int const* vtxsTagAroundIdx = &(_triangles[triTagAroundIdx * 3]);
						for(int j = 0; j < 3; ++j)
						{
							if(vtxsTagAroundIdx[j] == degenEdgeVtx1)
							{
								if(collectedTri.find(triTagAroundIdx) == collectedTri.end())
								{
									if(triOnOneSideOfDegenEdge.find(triTagAroundIdx) == triOnOneSideOfDegenEdge.end())
									{
										triToTag.push_back(triTagAroundIdx);
										triOnOneSideOfDegenEdge.insert(triTagAroundIdx);
										if(triAroundTouchingEdge.find(triTagAroundIdx) != triAroundTouchingEdge.end())
											triAroundTouchingEdge.erase(triTagAroundIdx);
									}
									break;
								}
							}
						}
					}
				}
			}
		}
		// Iterate around second edge vertex
		triToTag.push_back(refStartTriangle);
		collectedTri.clear();
		collectedTri.insert(triToTag[0]);
		while(!triToTag.empty())
		{
			// Iterate over the triangles vertices
			unsigned int const* vtxsTagIdx = &(_triangles[triToTag.back() * 3]);
			triToTag.pop_back();
			for(int i = 0; i < 3; ++i)
			{
				if((vtxsTagIdx[i] != degenEdgeVtx1) && (vtxsTagIdx[i] != degenEdgeVtx2))	// Vertex that doesn't belong to the degenerated edge
				{
					// Run through triangles around the vertex 
					std::vector<unsigned int> const& triTagAround = GetTrianglesAroundVertex(vtxsTagIdx[i]);
					for(unsigned int triTagAroundIdx : triTagAround)
					{
						// If the triangle has one of its vertex that belong to the degenrated edge, we can select it
						unsigned int const* vtxsTagAroundIdx = &(_triangles[triTagAroundIdx * 3]);
						for(int j = 0; j < 3; ++j)
						{
							if(vtxsTagAroundIdx[j] == degenEdgeVtx2)
							{
								if(collectedTri.find(triTagAroundIdx) == collectedTri.end())
								{
									if(triOnOneSideOfDegenEdge.find(triTagAroundIdx) == triOnOneSideOfDegenEdge.end())
									{
										triToTag.push_back(triTagAroundIdx);
										triOnOneSideOfDegenEdge.insert(triTagAroundIdx);
										if(triAroundTouchingEdge.find(triTagAroundIdx) != triAroundTouchingEdge.end())
											triAroundTouchingEdge.erase(triTagAroundIdx);
									}
									break;
								}
							}
						}
					}
				}
			}
		}
		if(triAroundTouchingEdge.size() == 2)
		{
			// Unlink triOnOneSideOfDegenEdge from the degenerated edge
			for(unsigned int triIdx : triOnOneSideOfDegenEdge)
			{
				unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
				for(int i = 0; i < 3; ++i)
				{
					if(vtxsIdx[i] == degenEdgeVtx1)
						RemoveTriangleAroundVertex(degenEdgeVtx1, triIdx);
					else if(vtxsIdx[i] == degenEdgeVtx2)
						RemoveTriangleAroundVertex(degenEdgeVtx2, triIdx);
				}
			}
			// Create two new vertex, at the same position of the degenerated vertices
			Vector3 newVtx1(_vertices[degenEdgeVtx1]);
			unsigned int newVtx1Id = AddVertex(newVtx1);
			if(retNewVerticesIdx != nullptr)
				retNewVerticesIdx->push_back(newVtx1Id);
			Vector3 newVtx2(_vertices[degenEdgeVtx2]);
			unsigned int newVtx2Id = AddVertex(newVtx2);
			if(retNewVerticesIdx != nullptr)
				retNewVerticesIdx->push_back(newVtx2Id);
			// Replace vertex index on triOnOneSideOfDegenEdge
			for(unsigned int triIdx : triOnOneSideOfDegenEdge)
			{
				unsigned int* vtxsIdx = &(_triangles[triIdx * 3]);
				for(int i = 0; i < 3; ++i)
				{
					unsigned int& vtxIdx = vtxsIdx[i];
					if(vtxIdx == degenEdgeVtx1)
					{
						vtxIdx = newVtx1Id;
						AddTriangleAroundVertex(newVtx1Id, triIdx);
					}
					else if(vtxIdx == degenEdgeVtx2)
					{
						vtxIdx = newVtx2Id;
						AddTriangleAroundVertex(newVtx2Id, triIdx);
					}
				}
			}
			// Remove degenerated quads and triangles
			DetectAndRemoveDegeneratedQuadsAroundEdge(degenEdgeVtx1, degenEdgeVtx2);
			DetectAndRemoveDegeneratedQuadsAroundEdge(newVtx1Id, newVtx2Id);
			DetectAndRemoveDegeneratedTrianglesAroundVertex(degenEdgeVtx1, retNewVerticesIdx);
			DetectAndRemoveDegeneratedTrianglesAroundVertex(degenEdgeVtx2, retNewVerticesIdx);
			DetectAndRemoveDegeneratedTrianglesAroundVertex(newVtx1Id, retNewVerticesIdx);
			DetectAndRemoveDegeneratedTrianglesAroundVertex(newVtx2Id, retNewVerticesIdx);
			// Prepare for next loop if there is
			otherWeldVertices.erase(degenEdgeVtx2);
			if(!otherWeldVertices.empty() && !triAround.empty())
			{	// Does the remaining otherWeldVertices has been moved to the other side of the split zone ?
				degenEdgeVtx2 = *(otherWeldVertices.begin());
				std::vector<unsigned int> testTriAroundTouchingEdge;
				for(unsigned int triIdx : triAround)
				{
					unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
					unsigned int nbVtxMatching = 0;
					for(int i = 0; i < 3; ++i)
					{
						unsigned int const& vtxIdx = vtxsIdx[i];
						unsigned int matchingIndex[2];
						if((vtxIdx == degenEdgeVtx1) || (vtxIdx == degenEdgeVtx2))
						{
							matchingIndex[nbVtxMatching] = i;
							++nbVtxMatching;
						}
						if(nbVtxMatching == 2)
						{
							testTriAroundTouchingEdge.push_back(triIdx);
							break;
						}
					}
				}
				if(testTriAroundTouchingEdge.empty())
					vtxToTestIdx = newVtx1Id;	// Was moved to the other side of the split zone
			}
		}
		else
		{
			EmergencyCleanup(degenEdgeVtx1, degenEdgeVtx2);
			return;
		}
#endif
	}
	if(retNewVerticesIdx.size() > newVerticesBefore) // If something was done
	{
		// Set recompute normal request 
		if(!IsVertexToBeRemoved(vtxToTestIdx))
			SetHasToRecomputeNormal(vtxToTestIdx);
		for(auto& pair : triVerticesOccurences)
		{
			if(pair.second > 2)
			{
				if(!IsVertexToBeRemoved(pair.first))
					SetHasToRecomputeNormal(pair.first);
			}
		}
		for(unsigned int newVtxId : retNewVerticesIdx)
		{
			if(!IsVertexToBeRemoved(newVtxId))
				SetHasToRecomputeNormal(newVtxId);
		}
	}
}

void Mesh::DetectAndRemoveDegeneratedTrianglesAroundVertex(unsigned int vtxToTestIdx, std::vector<unsigned int>& retNewVerticesIdx)
{
	if(IsVertexToBeRemoved(vtxToTestIdx))
		return;
	std::vector<unsigned int> const& triAroundVertex = GetTrianglesAroundVertex(vtxToTestIdx);
	// Find and remove triangles that have the same vertices index
	for(int i = 0; i < (int) triAroundVertex.size(); ++i)
	{
		unsigned int firstTriToCheck = triAroundVertex[i];
		unsigned int const* vtxsIdx1 = &(_triangles[firstTriToCheck * 3]);
		for(int j = i + 1; j < (int) triAroundVertex.size(); ++j)
		{
			if(i == j)
				continue;
			unsigned int secondTriToCheck = triAroundVertex[j];
			if(IsTriangleToBeRemoved(firstTriToCheck) && IsTriangleToBeRemoved(secondTriToCheck))
				continue;
			unsigned int const* vtxsIdx2 = &(_triangles[secondTriToCheck * 3]);
			unsigned int nbSimilar = 0;
			for(int k = 0; k < 3; ++k)
			{
				for(int l = 0; l < 3; ++l)
				{
					if(vtxsIdx1[k] == vtxsIdx2[l])
					{
						++nbSimilar;
						break;
					}
				}
			}
			if(nbSimilar == 3)
			{
				DeleteTriangle(firstTriToCheck);
				DeleteTriangle(secondTriToCheck);
				i = -1;	// start over again
				break;
			}
		}
	}
	// Detect that the vertex doesn't join two separate zones, like this ><
	if(!triAroundVertex.empty())
	{
		// We take the first triangle touching edge, and we will run through connectivity to find all the touching triangles on its side
		std::vector<unsigned int> triToTag;
		triToTag.push_back(*(triAroundVertex.begin()));
		std::set<unsigned int> triOnOneSideOfVertex;
		triOnOneSideOfVertex.insert(triToTag[0]);
		while(!triToTag.empty())
		{
			// Iterate over the triangles vertices
			unsigned int const* vtxsTagIdx = &(_triangles[triToTag.back() * 3]);
			triToTag.pop_back();
			for(int i = 0; i < 3; ++i)
			{
				if(vtxsTagIdx[i] != vtxToTestIdx)
				{
					// Run through triangles around the vertex 
					std::vector<unsigned int> const& triTagAround = GetTrianglesAroundVertex(vtxsTagIdx[i]);
					for(unsigned int triTagAroundIdx : triTagAround)
					{
						// If the triangle has one of its vertex that is the vertex we are working on, we can select it
						unsigned int const* vtxsTagAroundIdx = &(_triangles[triTagAroundIdx * 3]);
						for(int j = 0; j < 3; ++j)
						{
							if(vtxsTagAroundIdx[j] == vtxToTestIdx)
							{
								if(triOnOneSideOfVertex.find(triTagAroundIdx) == triOnOneSideOfVertex.end())
								{
									triToTag.push_back(triTagAroundIdx);
									triOnOneSideOfVertex.insert(triTagAroundIdx);
								}
								break;
							}
						}
					}
				}
			}
		}
		if(triOnOneSideOfVertex.size() < triAroundVertex.size())
		{	// Case detected ! Two closed zones joined by one common vertex
			// Unlink triOnOneSideOfVertex from the degenerated edge
			for(unsigned int triIdx : triOnOneSideOfVertex)
			{
				unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
				for(int i = 0; i < 3; ++i)
				{
					if(vtxsIdx[i] == vtxToTestIdx)
					{
						RemoveTriangleAroundVertex(vtxToTestIdx, triIdx);
						break;
					}
				}
			}
			// Create one new vertex, at the same position of the degenerated vertices
			Vector3 newVtx(_vertices[vtxToTestIdx]);
			unsigned int newVtxId = AddVertex(newVtx);
			retNewVerticesIdx.push_back(newVtxId);
			// Replace vertex index on triOnOneSideOfVertex
			for(unsigned int triIdx : triOnOneSideOfVertex)
			{
				unsigned int* vtxsIdx = &(_triangles[triIdx * 3]);
				for(int i = 0; i < 3; ++i)
				{
					unsigned int& vtxIdx = vtxsIdx[i];
					if(vtxIdx == vtxToTestIdx)
					{
						vtxIdx = newVtxId;
						AddTriangleAroundVertex(newVtxId, triIdx);
						break;
					}
				}
			}
			// Set recompute normal request 
			SetHasToRecomputeNormal(vtxToTestIdx);
			SetHasToRecomputeNormal(newVtxId);
		}
	}
}

void Mesh::DetectAndRemoveDegeneratedQuadsAroundEdge(unsigned int edgeVtx1Idx, unsigned int edgeVtx2Idx)
{	// Detect rwo quads facing one another (with nothing else around the edge), and delete it (note: will also remove 4 triangles pyramids... side effect, but usefull, so I keep it like that)
	std::vector<unsigned int> const& triAroundVtx1 = GetTrianglesAroundVertex(edgeVtx1Idx);
	std::vector<unsigned int> const& triAroundVtx2 = GetTrianglesAroundVertex(edgeVtx2Idx);
	if((triAroundVtx1.size() > 4) || (triAroundVtx2.size() > 4))
		return;
	std::set<unsigned int> allTrianglesAround;
	for(unsigned int const& triIdx : triAroundVtx1)
		allTrianglesAround.insert(triIdx);
	for(unsigned int const& triIdx : triAroundVtx2)
		allTrianglesAround.insert(triIdx);
	if(allTrianglesAround.size() == 4)	// If the edge have only a total 4 triangles around, this means they hold two quads facing one another.
	{
		// Just to be sure, we count the number of vertices held by those triangles, they must be four
		std::set<unsigned int> allTriangleVertices;
		for(unsigned int const& triIdx : allTrianglesAround)
		{
			unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
			for(int i = 0; i < 3; ++i)
				allTriangleVertices.insert(vtxsIdx[i]);
		}
		if(allTriangleVertices.size() == 4)
		{	// Ok do the deletion
			for(unsigned int const& triIdx : allTrianglesAround)
				DeleteTriangle(triIdx);
		}
	}
}

bool Mesh::CheckVertexIsClosed(unsigned int vtxIdx) // Returns true if the vertex is already completely surrounded by the triangles it's linked to
{
	for(unsigned int const& triIdx : _vtxToTriAround[vtxIdx])
	{
		unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
		// *** edge shared by 2 triangles only check ****
		for(int i = 0; i < 3; ++i)
		{
			if(vtxsIdx[i] != vtxIdx)
			{	// Test that an edge is only shared by two triangles
				std::vector<unsigned int> const& triAroundA = _vtxToTriAround[vtxIdx];
				std::vector<unsigned int> const& triAroundB = _vtxToTriAround[vtxsIdx[i]];
				unsigned int nbSharedTriangles = 0;
				for(unsigned int triIdxA : triAroundA)		// Todo : O(n), of course optimize this
				{
					for(unsigned int triIdxB : triAroundB)
					{
						if(triIdxA == triIdxB)
						{	// triangle common to vertex A and B
							++nbSharedTriangles;
							break;
						}
					}
				}
				if(nbSharedTriangles != 2)	//  Should be always the case on a manifold closed mesh
					return false;
			}
		}
	}
	return true;
}

bool Mesh::HasToDoAnEmergencyAutoSmooth()
{
	static int count = 0;
	for(unsigned int i = 0; i < _vtxsIdxToRecomputeNormalOn.size(); ++i)
	{
		unsigned int& vtxIdx = _vtxsIdxToRecomputeNormalOn[i];
		Vector3 refFaceNormal;
		bool hasRefFaceNormal = false;
		for(unsigned int const& triIdx : _vtxToTriAround[vtxIdx])
		{
			ASSERT(triIdx < _trisState.size());
			if(TestStateFlags(_trisState[triIdx], TRI_STATE_PENDING_REMOVE))
				continue;
			unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
			// *** flipped triangle check ****
			Vector3& vtx1 = _vertices[vtxsIdx[0]];
			Vector3& vtx2 = _vertices[vtxsIdx[1]];
			Vector3& vtx3 = _vertices[vtxsIdx[2]];
			Vector3 p1p2 = vtx2 - vtx1;
			Vector3 p1p3 = vtx3 - vtx1;
			Vector3 triNormal = p1p2.Cross(p1p3);
			float triNormalLength = triNormal.Length();
			if(triNormalLength != 0.0f)
			{
				triNormal /= triNormalLength;
				if(!hasRefFaceNormal)
				{	// Get the first face normal
					refFaceNormal = triNormal;
					hasRefFaceNormal = true;
				}
				else
				{	// Check if the angle is not too much, if so, this really might be a flipped face
					float dot = refFaceNormal.Dot(triNormal);
					if(dot <= cosf(179.0f * float(M_PI) / 180.0f))
					{
						printf("Emergency smooth %d\n", ++count);
						return true;
					}
				}
			}
		}
	}
	return false;
}

#ifdef _DEBUG
#include "MeshRecorder.h"
void Mesh::SaveMeshPieceForVisualDebugGivingVertices(std::vector<unsigned int> const& verticesToSaveTriAround, std::string filename)
{
	// Collect triangles
	std::set<unsigned int> trianglesSet;
	for(unsigned int inputVtxIdx : verticesToSaveTriAround)
	{
		for(unsigned int triAroundInputVtxIdx : _vtxToTriAround[inputVtxIdx])
			trianglesSet.insert(triAroundInputVtxIdx);
	}
	std::vector<unsigned int> trianglesVect(trianglesSet.begin(), trianglesSet.end());
	SaveMeshPieceForVisualDebugGivingTriangles(trianglesVect, filename);
}

void Mesh::SaveMeshPieceForVisualDebugGivingTriangles(std::vector<unsigned int> const& triangleToSave, std::string filename)
{
#ifdef MESH_CONSISTENCY_CHECK
	bool backupVar = consistencyCheckOn;
	consistencyCheckOn = false;	// Don't do consistency check here: we are building a partial mesh, so obvisouly most checks will fail
#endif // MESH_CONSISTENCY_CHECK
	// Build triangles indices
	std::vector<unsigned int> triangles;
	triangles.reserve(triangleToSave.size() * 3);
	for(unsigned int triIdx : triangleToSave)
	{
		triangles.push_back(_triangles[triIdx * 3]);
		triangles.push_back(_triangles[triIdx * 3 + 1]);
		triangles.push_back(_triangles[triIdx * 3 + 2]);
	}
	// Collect vertices from triangles, and build a map to remap vertices indices
	std::vector<Vector3> vertices;
	std::map<unsigned int, unsigned int> remapVtxIdx;
	for(unsigned int vtxIdx : triangles)
	{
		if(remapVtxIdx.find(vtxIdx) == remapVtxIdx.end())
		{	// Doesn't exist yet, create it
			remapVtxIdx[vtxIdx] = (unsigned int) vertices.size();
			vertices.push_back(_vertices[vtxIdx]);
		}
	}
	// Remap triangles vertices indices
	for(unsigned int& vtxIdx : triangles)
		vtxIdx = remapVtxIdx[vtxIdx];
	// Build mesh
	Mesh mesh(triangles, vertices, -1, true, false, false, false, false);
	MeshRecorder meshRecorder;
	meshRecorder.SaveToFile(mesh, std::string("c:\\data\\debug\\") + filename);
#ifdef MESH_CONSISTENCY_CHECK
	consistencyCheckOn = backupVar;
#endif // MESH_CONSISTENCY_CHECK
}
#endif // _DEBUG

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(Mesh)
{
	register_vector<BBox>("BBoxVector");
	class_<Mesh>("Mesh")
		.function("Triangles", &Mesh::Triangles)
		.function("Vertices", &Mesh::Vertices)
		.function("Normals", &Mesh::Normals)
		.function("GetClosestIntersectionPoint", &Mesh::GetClosestIntersectionPoint, allow_raw_pointers())
		.function("GetFragmentsBBox", &Mesh::GetFragmentsBBox)
		.function("GetBBox", &Mesh::GetBBox)
		.function("IsManifold", &Mesh::IsManifold)
		.function("CanUndo", &Mesh::CanUndo)
		.function("Undo", &Mesh::Undo)
		.function("CanRedo", &Mesh::CanRedo)
		.function("Redo", &Mesh::Redo)
		.function("CSGMerge", &Mesh::CSGMerge)
		.function("CSGSubtract", &Mesh::CSGSubtract)
		.function("CSGIntersect", &Mesh::CSGIntersect)
		.function("GetSubMeshCount", &Mesh::GetSubMeshCount)
		.function("GetSubMesh", &Mesh::GrabSubMesh, allow_raw_pointers())
		.function("IsSubMeshExist", &Mesh::IsSubMeshExist)
		.function("UpdateSubMeshes", &Mesh::UpdateSubMeshes);
}
#endif // __EMSCRIPTEN__