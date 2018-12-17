#ifndef _CSG_H_
#define _CSG_H_

#define USE_BSP	// If not defined, will use refinement + stitching
//#define DO_STITCH_INTERSECTION_ZONE
#define DO_STITCH_WHOLE_MESH

#include <memory>
#include <vector>
#include <map>
#include "OctreeVisitor.h"
#include "Collisions\BBox.h"
#include "Collisions\BSphereDouble.h"
#ifdef USE_BSP
#include "CSG_Bsp.h"
#else
#include "Loop.h"
#endif //!USE_BSP

class Mesh;

class Csg
{
	class DetectOctreeBBoxIntersection: public OctreeVisitor
	{
	public:
		DetectOctreeBBoxIntersection(BBox const& collider): _collider(collider), _collides(false) {}
		bool Collides() const { return _collides; }
		virtual bool HasToVisit(OctreeCell& cell);
		virtual void VisitEnter(OctreeCell& /*cell*/) {}
		virtual void VisitLeave(OctreeCell& /*cell*/) {}

	private:
		BBox const& _collider;
		bool _collides;
	};

	class VisitorGetOctreeBBoxIntersection: public OctreeVisitor
	{
	public:
		VisitorGetOctreeBBoxIntersection(BBox const& collider): _collider(collider) {}
		std::vector<OctreeCell const*> const& GetCollidedCells() const { return _collidedCells; }
		virtual bool HasToVisit(OctreeCell& cell);
		virtual void VisitEnter(OctreeCell& cell);
		virtual void VisitLeave(OctreeCell& /*cell*/) {}

	private:
		BBox const& _collider;
		std::vector<OctreeCell const*> _collidedCells;
	};

	class VisitorGatherCollidingCells: public OctreeVisitor
	{
	public:
		VisitorGatherCollidingCells(Mesh& meshToCollideWith): _meshToCollideWith(meshToCollideWith) {}
		virtual bool HasToVisit(OctreeCell& cell);
		virtual void VisitEnter(OctreeCell& /*cell*/) {}
		virtual void VisitLeave(OctreeCell& /*cell*/) {}
		std::map<OctreeCell const*, std::vector<OctreeCell const*>> const& GetColliderToCollidingCells() { return _colliderToCollidingCells; }

	private:
		Mesh& _meshToCollideWith;
		std::map<OctreeCell const*, std::vector<OctreeCell const*>> _colliderToCollidingCells;
	};

public:
	enum CSG_OPERATION: unsigned char
	{
		MERGE,
		SUBTRACT,
		INTERSECT
	};
	// Note: resultMesh could be the firstMesh or secondMesh
	Csg(Mesh& firstMesh, Mesh& secondMesh, Mesh& resultMesh): _firstMesh(firstMesh), _secondMesh(secondMesh), _resultMesh(resultMesh) {}
	bool ComputeCSGOperation(CSG_OPERATION operation, bool recenter);	// Return false if failed

private:
	void ExtractRemainingMeshTess(Mesh& inMesh, std::vector<Vector3>& outVertices, std::vector<unsigned int>& outTriangles);
	void FlipMesh(Mesh& mesh);
	void InvalidateInsideMeshPart(Mesh& mesh, Mesh& otherMesh, std::vector<unsigned int> const& intersectionTriangles);
	void RebuildMeshFromCsg(bool recenter);
#ifdef USE_BSP	
	Csg_Bsp BuildCSGBsp(std::vector<unsigned int> trianglesIdx, Mesh& mesh);
	void AddToCsgResult(Csg_Bsp elementToAdd);
#ifdef DO_STITCH_INTERSECTION_ZONE
	void StitchIntersectionZone();
	void CheckVertexIsCorrect(unsigned int vtxIdx);
#endif // DO_STITCH_INTERSECTION_ZONE
#else
	void PairLoops();
	void StitchProgress();
	void StitchFinalize();
#endif // !USE_BSP

	Mesh& _firstMesh;
	Mesh& _secondMesh;
	Mesh& _resultMesh;
#ifdef USE_BSP
	std::map<unsigned int, std::vector<unsigned int> > _firstMeshTriTouchesSecondMeshTris;
	std::map<unsigned int, std::vector<unsigned int> > _secondMeshTriTouchesFirstMeshTris;
	std::vector<Vector3Double> _csgBspVerticesResult;
	std::vector<unsigned int> _csgBspTrianglesResult;
#ifdef DO_STITCH_INTERSECTION_ZONE
	std::vector<BSphereDouble> _csgBspTrianglesBSphere;
	std::vector<std::vector<unsigned int> > _csgBspVtxToTriAround;
	std::vector<bool> _csgBspVtxsToBeRemoved;

	void CsgBspAddTriangleAroundVertex(unsigned int vertexIndex, unsigned int triangleIndex)
	{
#ifdef _DEBUG
		for(unsigned int triIdx : _csgBspVtxToTriAround[vertexIndex])
			ASSERT(triIdx != triangleIndex);	// Verify the triangle index we add doesn't already exist
#endif // _DEBUG
		_csgBspVtxToTriAround[vertexIndex].push_back(triangleIndex);
	}

	void CsgBspChangeTriangleAroundVertex(unsigned int vertexIndex, unsigned int oldTtriangleIndex, unsigned int newTtriangleIndex)
	{
		// Todo: build a map or something to prevent the for loop
		for(unsigned int& tri : _csgBspVtxToTriAround[vertexIndex])
		{
			if(tri == oldTtriangleIndex)
			{
				tri = newTtriangleIndex;
				return;
			}
		}
		ASSERT(false);	// Means we didn't find oldTtriangleIndex 
	}
	void CsgBspRemoveTriangleAroundVertex(unsigned int vertexIndex, unsigned int triangleIndex)
	{
		// Todo: build a map or something to prevent the for loop
		for(unsigned int& tri : _csgBspVtxToTriAround[vertexIndex])
		{
			if(tri == triangleIndex)
			{
				tri = _csgBspVtxToTriAround[vertexIndex].back();
				_csgBspVtxToTriAround[vertexIndex].pop_back();
				ASSERT((_csgBspVtxToTriAround[vertexIndex].empty() == false) || _csgBspVtxsToBeRemoved[vertexIndex]);
				return;
			}
		}
		ASSERT(_csgBspVtxsToBeRemoved[vertexIndex]);	// We get there if we didn't find triangleIndex in _vtxToTriAround
	}
	void CsgBspClearTriangleAroundVertex(unsigned int vertexIndex) { _csgBspVtxToTriAround[vertexIndex].clear(); }

	unsigned int CsgBspAddTriangle(unsigned int vtx1, unsigned int vtx2, unsigned int vtx3)
	{
		unsigned int nbTri = (unsigned int) _csgBspTrianglesBSphere.size();
		ASSERT(nbTri == _csgBspTrianglesResult.size() / 3);
		_csgBspTrianglesResult.push_back(vtx1);
		_csgBspTrianglesResult.push_back(vtx2);
		_csgBspTrianglesResult.push_back(vtx3);
		_csgBspTrianglesBSphere.push_back(BSphereDouble(_csgBspVerticesResult[vtx1], _csgBspVerticesResult[vtx2], _csgBspVerticesResult[vtx3]));
		return nbTri;
	}

	void CsgBspSetVertexToBeRemoved(unsigned int vtxIdx)
	{
		_csgBspVtxsToBeRemoved[vtxIdx] = true;
	}

	bool CsgBspIsVertexToBeRemoved(unsigned int vtxIdx)
	{
		return _csgBspVtxsToBeRemoved[vtxIdx];
	}
#endif // DO_STITCH_INTERSECTION_ZONE

#else
	std::vector<Loop> _firstMeshLoops;
	std::vector<Loop> _secondMeshLoops;
	std::map<Loop*, Loop*> _firstToSecondMeshLoopPairs;
#endif // !USE_BSP
	std::vector<unsigned int> _resultTriangles;
	std::vector<Vector3> _resultVertices;
};

#endif // _CSG_H_
