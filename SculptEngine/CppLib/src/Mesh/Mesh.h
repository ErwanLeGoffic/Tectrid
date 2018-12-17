#ifndef _MESH_H_
#define _MESH_H_

#include <vector>
#include <deque>
#include "Math\Math.h"
#include "Math\Vector.h"
#include "Math\Matrix.h"
#include "Collisions\Ray.h"
#include "Collisions\BBox.h"
#include "Collisions\BSphere.h"
#include "Octree.h"
#include "OctreeVisitorCollectBBox.h"
#include "OctreeVisitorBuildAndCollectSubMeshes.h"
#include "CSG.h"
#include "Retessellate.h"

#ifdef __EMSCRIPTEN__ 
#include <emscripten/val.h>
#endif // __EMSCRIPTEN__

#ifdef _DEBUG
//#define MESH_CONSISTENCY_CHECK
#ifdef MESH_CONSISTENCY_CHECK
extern bool consistencyCheckOn;
#endif // MESH_CONSISTENCY_CHECK
#endif // _DEBUG

class SubMesh;
class VisitorRetessellateInRange;
class LoopBuilder;
class LoopElement;
class Retessellate;

//#define CLEAN_PENDING_REMOVALS_IMMEDIATELY	// Much slower if turned on

const float smoothGroupCosLimit = cosf(40.0f * float(M_PI) / 180.0f);

class VisitorGetOctreeVertexIntersection: public OctreeVisitor
{
public:
	VisitorGetOctreeVertexIntersection(Vector3 const& collider): _collider(collider, EPSILON) {}
	std::vector<OctreeCell const*> const& GetCollidedCells() const { return _collidedCells; }
	virtual bool HasToVisit(OctreeCell& cell);
	virtual void VisitEnter(OctreeCell& cell);
	virtual void VisitLeave(OctreeCell& /*cell*/) {}

private:
	BSphere _collider;
	std::vector<OctreeCell const*> _collidedCells;
};

class Mesh
{
	friend class Csg;	// Todo: try to remove the friend if possible
public:
	Mesh(std::vector<unsigned int>& triangles, std::vector<Vector3>& vertices, int id, bool freeInputBuffers, bool rescale, bool recenter, bool buildHardEdges, bool weldVertices);
	Mesh(Mesh const& otherMesh, bool copySnapshots, bool copyOctree);	// If copyOctree is false, it'll be let empty in the created object

	int GetID() const { return _id; }

	// Mesh related
	bool IsManifold();
	void RecomputeNormals(bool forceReducedCompute, bool authorizeAutoSmooth = true);
	void ProcessHardEdges(std::vector<unsigned int>* createdTriangles, std::vector<unsigned int>* createdVertices);
	void ReBalanceOctree(std::vector<unsigned int> const& additionalTrisToInsert, std::vector<unsigned int> const& additionalVtxsToInsert, bool extractFromAllCells);
	void HandlePendingRemovals();
	void TagAndCollectOpenEdgesVertices(LoopBuilder *loopBuilder);
	bool CheckVertexIsClosed(unsigned int vtxIdx);
#ifdef _DEBUG
	void SaveMeshPieceForVisualDebugGivingVertices(std::vector<unsigned int> const& verticesToSaveTriAround, std::string filename);
	void SaveMeshPieceForVisualDebugGivingTriangles(std::vector<unsigned int> const& triangleToSave, std::string filename);
#endif // _DEBUG

	// Collision related
	bool GetClosestIntersectionPoint(Ray const& ray, Vector3& point, Vector3* normal, bool cullBackFace);	// "normal" could be null (optional)
	bool GetClosestIntersectionPointAndTriangle(Ray const& ray, Vector3& point, unsigned int* triangle, Vector3* normal, bool cullBackFace);	// "triangle" and "normal" could be null (optional)

	// Vertices related
	std::vector<Vector3> const& GetVertices() const { return _vertices; }
	std::vector<Vector3>& GrabVertices() { return _vertices; }

	std::vector<Vector3> const& GetNormals() const { return _vtxsNormal; }
	std::vector<Vector3>& GrabNormals() { return _vtxsNormal; }

	void AddTriangleAroundVertex(unsigned int vertexIndex, unsigned int triangleIndex)
	{
#ifdef _DEBUG
		for(unsigned int triIdx : _vtxToTriAround[vertexIndex])
			ASSERT(triIdx != triangleIndex);	// Verify the triangle index we add doesn't already exist
#endif // _DEBUG
		_vtxToTriAround[vertexIndex].push_back(triangleIndex);
	}

	void ChangeTriangleAroundVertex(unsigned int vertexIndex, unsigned int oldTtriangleIndex, unsigned int newTtriangleIndex)
	{
		// Todo: build a map or something to prevent the for loop
		for(unsigned int& tri : _vtxToTriAround[vertexIndex])
		{
			if(tri == oldTtriangleIndex)
			{
				tri = newTtriangleIndex;
				return;
			}
		}
		ASSERT(false);	// Means we didn't find oldTtriangleIndex 
	}
	void RemoveTriangleAroundVertex(unsigned int vertexIndex, unsigned int triangleIndex)
	{
		// Todo: build a map or something to prevent the for loop
		for(unsigned int& tri : _vtxToTriAround[vertexIndex])
		{
			if(tri == triangleIndex)
			{
				tri = _vtxToTriAround[vertexIndex].back();
				_vtxToTriAround[vertexIndex].pop_back();
				ASSERT((_vtxToTriAround[vertexIndex].empty() == false) || TestStateFlags(_vtxsState[vertexIndex], VTX_STATE_PENDING_REMOVE));
				return;
			}
		}
		ASSERT(TestStateFlags(_vtxsState[vertexIndex], VTX_STATE_PENDING_REMOVE));	// We get there if we didn't find triangleIndex in _vtxToTriAround
	}
	std::vector<unsigned int> const& GetTrianglesAroundVertex(unsigned int vertexIndex) const { return _vtxToTriAround[vertexIndex]; }

	void ClearTriangleAroundVertex(unsigned int vertexIndex) { _vtxToTriAround[vertexIndex].clear(); }

	void SetHasToRecomputeNormal(unsigned int vertexIndex);

	unsigned int AddVertex(Vector3 const& newVertex)
	{
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
		if(_vtxsIdxToRecycle.empty() == false)
		{	// We recycle
			unsigned int recycledVtxId = _vtxsIdxToRecycle.back();
			_vtxsIdxToRecycle.pop_back();
			ASSERT(TestStateFlags(_vtxsState[recycledVtxId], VTX_STATE_PENDING_REMOVE));
			_vertices[recycledVtxId] = newVertex;
			_vtxToTriAround[recycledVtxId].clear();
			_vtxsNormal[recycledVtxId].ResetToZero();
			_vtxsState[recycledVtxId] = 0;
			_vtxsNewIdx[recycledVtxId] = UNDEFINED_NEW_ID;
			return recycledVtxId;
		}
		else
		{
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
			unsigned int nbVtx = (unsigned int) _vertices.size();
			_vertices.push_back(newVertex);
			_vtxToTriAround.resize(nbVtx + 1);
			_vtxsNormal.resize(nbVtx + 1);
			_vtxsState.resize(nbVtx + 1);
			_vtxsNewIdx.push_back(UNDEFINED_NEW_ID);
			return nbVtx;
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
		}
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
	}

	bool VtxHasToMove(unsigned int vtxId) const
	{
		return _vtxsNewIdx[vtxId] != UNDEFINED_NEW_ID;
	}

	unsigned int GetNewVtxIdx(unsigned int vtxId) const
	{
		return _vtxsNewIdx[vtxId];
	}

	bool IsVertexOnOpenEdge(unsigned int vtxId) const { return TestStateFlags(_vtxsState[vtxId], VTX_STATE_IS_ON_OPEN_EDGE); }
	void SetVertexOnOpenEdge(unsigned int vtxId) { AddStateFlags(_vtxsState[vtxId], VTX_STATE_IS_ON_OPEN_EDGE);	}
	void ClearAllVerticesOnOpenEdge() { for(unsigned char& vtxState : _vtxsState) ClearStateFlags(vtxState, VTX_STATE_IS_ON_OPEN_EDGE); }

	bool IsVertexInStitchingZone(unsigned int vtxId) const { return TestStateFlags(_vtxsState[vtxId], VTX_STATE_IS_IN_STITCHING_ZONE); }
	void SetVertexInStitchingZone(unsigned int vtxId) { AddStateFlags(_vtxsState[vtxId], VTX_STATE_IS_IN_STITCHING_ZONE); }
	void ClearAllVerticesInStitchingZone() { for(unsigned char& vtxState : _vtxsState) ClearStateFlags(vtxState, VTX_STATE_IS_IN_STITCHING_ZONE); }

	bool IsVertexToUpdateInSubMesh(unsigned int vtxId) const { return TestStateFlags(_vtxsState[vtxId], VTX_STATE_HAS_TO_RECOMPUTE_SUB_MESH); }
	void ClearAllVerticesToUpdateInSubMesh() { for(unsigned char& vtxState : _vtxsState) ClearStateFlags(vtxState, VTX_STATE_HAS_TO_RECOMPUTE_SUB_MESH); }

	bool IsVertexAlreadyTreated(unsigned int vtxId) const { return TestStateFlags(_vtxsState[vtxId], VTX_STATE_ALREADY_TREATED); }
	void SetVertexAlreadyTreated(unsigned int vtxId) { AddStateFlags(_vtxsState[vtxId], VTX_STATE_ALREADY_TREATED); }
	void ClearVertexAlreadyTreated(unsigned int vtxId) { ClearStateFlags(_vtxsState[vtxId], VTX_STATE_ALREADY_TREATED); }
	void ClearAllVerticesAlreadyTreated() { for(unsigned char& vtxState : _vtxsState) ClearStateFlags(vtxState, VTX_STATE_ALREADY_TREATED); }
#ifdef _DEBUG
	bool IsThereSomeVerticesWithTreatedFlag() { for(unsigned char& vtxState : _vtxsState) if(TestStateFlags(vtxState, VTX_STATE_ALREADY_TREATED)) return true; return false; }
#endif // _DEBUG

	void ClearAllVerticesInStitchingZoneAndAlreadyTreated() { for(unsigned char& vtxState : _vtxsState) ClearStateFlags(vtxState, VTX_STATE_ALREADY_TREATED | VTX_STATE_IS_IN_STITCHING_ZONE); }

	bool IsVertexToBeRemoved(unsigned int vtxId) const { return TestStateFlags(_vtxsState[vtxId], VTX_STATE_PENDING_REMOVE); }
	void SetVertexToBeRemoved(unsigned int vtxId)
	{
		ASSERT(!IsVertexToBeRemoved(vtxId));
		if(IsVertexToBeRemoved(vtxId))
		{
			printf("!!! Trying to remove vertex %d. Which is already removed !!!\n", vtxId);
			return;
		}
		AddStateFlags(_vtxsState[vtxId], VTX_STATE_PENDING_REMOVE);
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
		_vtxsIdxToRemove.push_back(vtxId);
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
	}

	void DeleteVertexAndReconnectTrianglesToAnotherOne(unsigned int vtxIdxToDelete, unsigned int vtxIdxToReconnectTo);

	// Triangles related
	std::vector<unsigned int> const& GetTriangles() const { return _triangles; }
	std::vector<unsigned int>& GrabTriangles() { return _triangles; }

	std::vector<Vector3> const& GetTrisNormal() const { return _trisNormal; }
	std::vector<Vector3>& GrabTrisNormal() { return _trisNormal; }

	std::vector<BSphere> const& GetTrisBSphere() const { return _trisBSphere; }	
	std::vector<BSphere>& GrabTrisBSphere() { return _trisBSphere; }

	unsigned int AddTriangle(unsigned int vtx1, unsigned int vtx2, unsigned int vtx3, Vector3 const& normal, bool computeBSphere)
	{
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
		if(_trisIdxToRecycle.empty() == false)
		{	// We recycle
			unsigned int recycledTriId = _trisIdxToRecycle.back();
			_trisIdxToRecycle.pop_back();
			ASSERT(TestStateFlags(_trisState[recycledTriId], TRI_STATE_PENDING_REMOVE));
			unsigned int *vtxIdx = &(_triangles[recycledTriId * 3]);
			vtxIdx[0] = vtx1;
			vtxIdx[1] = vtx2;
			vtxIdx[2] = vtx3;
			_trisNormal[recycledTriId] = normal;
			if(computeBSphere)
				_trisBSphere[recycledTriId] = (BSphere(_vertices[vtx1], _vertices[vtx2], _vertices[vtx3]));
			else
				_trisBSphere[recycledTriId] = BSphere();
			_trisState[recycledTriId] = 0;
			_trisNewIdx[recycledTriId] = UNDEFINED_NEW_ID;
			return recycledTriId;
		}
		else
		{
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
			unsigned int nbTri = (unsigned int) _trisNormal.size();
			_triangles.push_back(vtx1);
			_triangles.push_back(vtx2);
			_triangles.push_back(vtx3);
			_trisNormal.push_back(normal);
			if(computeBSphere)
				_trisBSphere.push_back(BSphere(_vertices[vtx1], _vertices[vtx2], _vertices[vtx3]));
			else
				_trisBSphere.push_back(BSphere());
			_trisState.push_back(0);
			_trisNewIdx.push_back(UNDEFINED_NEW_ID);
			return nbTri;
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
		}
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
	}

	bool TriHasToMove(unsigned int triId) const
	{
		return _trisNewIdx[triId] != UNDEFINED_NEW_ID;
	}

	unsigned int GetNewTriIdx(unsigned int triId) const
	{
		return _trisNewIdx[triId];
	}

	bool IsTriangleToBeRemoved(unsigned int triId) const { return TestStateFlags(_trisState[triId], TRI_STATE_PENDING_REMOVE); }
	void SetTriangleToBeRemoved(unsigned int triId)
	{
		ClearStateFlags(_trisState[triId], TRI_STATE_HAS_TO_RECOMPUTE_NORMAL);
		AddStateFlags(_trisState[triId], TRI_STATE_PENDING_REMOVE);
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
		_trisIdxToRemove.push_back(triId);
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
	}

	bool RemoveFlatTriangle(unsigned int triIdx);	// Returns true if the triangle was removed
	void RemoveFlatTriangles() { for(unsigned int triIdx = 0; triIdx < _trisState.size(); ++triIdx) RemoveFlatTriangle(triIdx); }
	void DeleteTriangle(unsigned int triIdx);

	bool IsTriangleAlreadyTreated(unsigned int triId) const { return TestStateFlags(_trisState[triId], TRI_STATE_ALREADY_TREATED); }
	void SetTriangleAlreadyTreated(unsigned int triId) { AddStateFlags(_trisState[triId], TRI_STATE_ALREADY_TREATED); }
	void ClearTriangleAlreadyTreated(unsigned int triId) { ClearStateFlags(_trisState[triId], TRI_STATE_ALREADY_TREATED); }
	void ClearAllTrianglesAlreadyTreated() { for(unsigned char& triState : _trisState) ClearStateFlags(triState, TRI_STATE_ALREADY_TREATED); }
#ifdef _DEBUG
	bool IsThereSomeTrianglesWithTreatedFlag() { for(unsigned char& triState : _trisState) if(TestStateFlags(triState, TRI_STATE_ALREADY_TREATED)) return true; return false; }
#endif // _DEBUG

	bool CanTriangleBeRetessellated(unsigned int triId) const { return !TestStateFlags(_trisState[triId], TRI_STATE_DONT_RETESSELATE); }
	void BanTriangleRetessellation(unsigned int triId) { AddStateFlags(_trisState[triId], TRI_STATE_DONT_RETESSELATE); }
	void ClearTriangleRetessellationBan(unsigned int triId) { ClearStateFlags(_trisState[triId], TRI_STATE_DONT_RETESSELATE); }
	void ClearAllTriangleRetessellationBan() { for(unsigned char& triState : _trisState) ClearStateFlags(triState, TRI_STATE_DONT_RETESSELATE); }

	// Vertices and triangles related
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
	void TransferPendingRemovesToRecycle()
	{
		_vtxsIdxToRecycle.insert(_vtxsIdxToRecycle.end(), _vtxsIdxToRemove.begin(), _vtxsIdxToRemove.end());
		_vtxsIdxToRemove.clear();
		_trisIdxToRecycle.insert(_trisIdxToRecycle.end(), _trisIdxToRemove.begin(), _trisIdxToRemove.end());
		_trisIdxToRemove.clear();
	}
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY

	// Bbox related
	BBox const& GetBBox() const;

	void RecomputeFragmentsBBox(bool forceAllCellRecomputing);

	std::vector<BBox> GetFragmentsBBox()
	{
		VisitorCollectBBox collectBbox;
		_octreeRoot->Traverse(collectBbox);
		return std::move(collectBbox.GetBBoxes());
	}

	// Octree related
	OctreeCell& GrabOctreeRoot() { ASSERT(_octreeRoot.get() != nullptr); return *(_octreeRoot.get()); }

	// Submesh related
	unsigned int GetSubMeshCount() { return GrabSubMeshesVisitor().GetSubMeshCount(); }
	SubMesh const* GetSubMesh(unsigned int index) { return GrabSubMeshesVisitor().GetSubMesh(index); }
	SubMesh* GrabSubMesh(unsigned int index) { return GrabSubMeshesVisitor().GrabSubMesh(index); }
	bool IsSubMeshExist(unsigned int subMeshID) { return GrabSubMeshesVisitor().IsSubMeshExist(subMeshID); }
	void UpdateSubMeshes() { GrabOctreeRoot().Traverse(GrabSubMeshesVisitor()); }

	// Undo/redo related
	bool CanUndo();
	bool Undo();
	bool CanRedo();
	bool Redo();
	void TakeSnapShot();

	// Transform related
	void Transform(Matrix3 const& rotAndScale, Vector3 const& position);

	// CSG related
	bool CSGTest();
	bool CSGMerge(Mesh& otherMesh, Matrix3 const& rotAndScale, Vector3 const& position, bool recenterResult);
	bool CSGSubtract(Mesh& otherMesh, Matrix3 const& rotAndScale, Vector3 const& position, bool recenterResult);
	bool CSGIntersect(Mesh& otherMesh, Matrix3 const& rotAndScale, Vector3 const& position, bool recenterResult);

	// Degenerated handling
	void DetectAndTreatDegeneratedEdgeAroundVertex(unsigned int vtxToTestIdx, std::vector<unsigned int>& retNewVerticesIdx);
	void DetectAndRemoveDegeneratedTrianglesAroundVertex(unsigned int vtxToTestIdx, std::vector<unsigned int>& retNewVerticesIdx);
	void DetectAndRemoveDegeneratedQuadsAroundEdge(unsigned int edgeVtx1Idx, unsigned int edgeVtx2Idx);

	// Retessellate related
	Retessellate& GrabRetessellator()
	{
		if(_retessellator == nullptr)
			_retessellator.reset(new Retessellate(*this));	// Lazy init
		return *_retessellator;
	}

#ifdef __EMSCRIPTEN__ 
	emscripten::val Triangles() { return emscripten::val(emscripten::typed_memory_view(_triangles.size() * sizeof(int), (char const *) _triangles.data())); }
	emscripten::val Vertices() { return emscripten::val(emscripten::typed_memory_view(_vertices.size() * sizeof(Vector3), (char const *) _vertices.data())); }
	emscripten::val Normals() { return emscripten::val(emscripten::typed_memory_view(_vtxsNormal.size() * sizeof(Vector3), (char const *) _vtxsNormal.data())); }
#endif // __EMSCRIPTEN__ 

private:
	enum VTX_STATE_FLAGS: unsigned char
	{
		VTX_STATE_HAS_TO_RECOMPUTE_NORMAL = 1,
		VTX_STATE_ALREADY_TREATED = VTX_STATE_HAS_TO_RECOMPUTE_NORMAL << 1,	// used for example in "VisitorBuildAndCollectSubMeshes"
		VTX_STATE_PENDING_REMOVE = VTX_STATE_ALREADY_TREATED << 1,
		VTX_STATE_HAS_TO_RECOMPUTE_SUB_MESH = VTX_STATE_PENDING_REMOVE << 1,
		VTX_STATE_IS_ON_OPEN_EDGE = VTX_STATE_HAS_TO_RECOMPUTE_SUB_MESH << 1,
		VTX_STATE_IS_IN_STITCHING_ZONE = VTX_STATE_IS_ON_OPEN_EDGE << 1
	};

	enum TRI_STATE_FLAGS: unsigned char
	{
		TRI_STATE_HAS_TO_RECOMPUTE_NORMAL = 1,
		TRI_STATE_PENDING_REMOVE = TRI_STATE_HAS_TO_RECOMPUTE_NORMAL << 1,
		TRI_STATE_ALREADY_TREATED = TRI_STATE_PENDING_REMOVE << 1,
		TRI_STATE_DONT_RETESSELATE = TRI_STATE_ALREADY_TREATED << 1
	};

	// State flag related
	void AddStateFlags(unsigned char& state, unsigned char flags) { state |= flags; }
	void ClearStateFlags(unsigned char& state, unsigned char flags) { state &= ~flags; }
	bool TestStateFlags(unsigned char const& state, unsigned char flags) const { return (state & flags) != 0; }

	// Mesh related
	void WeldVertices(std::vector<unsigned int> const& triIn, std::vector<Vector3> const& vtxsIn, std::vector<unsigned int>& triOut, std::vector<Vector3>& vtxsOut);
	void RebuildMeshData(bool rescale, bool recenter, bool buildHardEdges);
#ifdef MESH_CONSISTENCY_CHECK
	public:
	void CheckTriangleIsCorrect(unsigned int triIdx, bool testFlateness);
	void CheckVertexIsCorrect(unsigned int vtxIdx);
	void CheckMeshIsCorrect();
#endif // MESH_CONSISTENCY_CHECK

public:
	// Octree related
	void BuildOctree(BBox bbox);
private:
	// Sub meshes related
	VisitorBuildAndCollectSubMeshes& GrabSubMeshesVisitor()
	{
		if(_subMeshesVisitor == nullptr)
			_subMeshesVisitor.reset(new VisitorBuildAndCollectSubMeshes(*this));	// Lazy init
		return *_subMeshesVisitor;
	}

	// Undo/redo related
	void RestoreMeshToCurSnapshot();

	// AutoSmooth related
	bool HasToDoAnEmergencyAutoSmooth();

	// Vertices related
	std::vector<Vector3> _vertices;
	std::vector<unsigned char> _vtxsState;	// See VTX_STATE_FLAGS
	std::vector<unsigned int> _vtxsNewIdx;	// Store the new index "to be" of the vertex as it will be moved somewhere else
	std::vector<Vector3> _vtxsNormal;
	std::vector<std::vector<unsigned int> > _vtxToTriAround;	// For each vertex, tells the triangles around it
	// Triangles related
	std::vector<unsigned int> _triangles;	// 3 int per triangle (3 vertex index)
	std::vector<unsigned char> _trisState;	// See TRI_STATE_FLAGS
	std::vector<unsigned int> _trisNewIdx;	// Store the new index "to be" of the triangle as it will be moved somewhere else
	std::vector<Vector3> _trisNormal;
	std::vector<BSphere> _trisBSphere;	// For fast triangle distance pre-tests
	// Reduced recompute normal related
	std::vector<unsigned int> _vtxsIdxToRecomputeNormalOn;
	std::vector<unsigned int> _trisIdxToRecomputeNormalOn;
#ifndef CLEAN_PENDING_REMOVALS_IMMEDIATELY
	// Pending Remove related
	std::vector<unsigned int> _vtxsIdxToRemove;
	std::vector<unsigned int> _trisIdxToRemove;
	std::vector<unsigned int> _vtxsIdxToRecycle;
	std::vector<unsigned int> _trisIdxToRecycle;
#endif // !CLEAN_PENDING_REMOVALS_IMMEDIATELY
	// Octree related
	std::unique_ptr<OctreeCell> _octreeRoot;
	// Sub meshes related (this is for the moment only used to inject a split mesh into unity (due to its limit at 64K vertices per mesh))
	std::unique_ptr<VisitorBuildAndCollectSubMeshes> _subMeshesVisitor;
	// ID
	int _id;
	// Debug
	std::vector<Vector3> DEBUG_intersectionPoints;
	// Undo/redo related
	std::deque<std::vector<Vector3>> _verticesSnapshots;
	std::deque<std::vector<unsigned int>> _trianglesSnapshots;
	unsigned int _snapshotNextPos;
	// Consistency related
	bool _IsOpen;
	bool _IsManifold;
	// Retessalate related
	std::unique_ptr<Retessellate> _retessellator;
};

#endif // _MESH_H_