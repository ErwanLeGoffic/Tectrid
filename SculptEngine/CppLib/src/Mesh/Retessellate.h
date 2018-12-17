#ifndef _RETESSELLATE_H_
#define _RETESSELLATE_H_

#include <vector>
#include "OctreeVisitor.h"
#include "Math\Vector.h"
#include "Loop.h"

class Mesh;
class Retessellate
{
public:
	Retessellate(Mesh& mesh);

	void SetMaxEdgeLength(float value);
	void HandlePendingRemovals();
	std::vector<unsigned int> const& GetGenratedTris() const { return _newTrianglesIDs; }
	std::vector<unsigned int> const& GetGenratedVtxs() const { return _newVerticesIDs; }
	bool WasSomethingMerged() const { return _somethingWasMerged;	}
	void SetSomethingMerged(bool value) { _somethingWasMerged = value; }
	bool WasSomethingSubdivided() const { return _somethingWasSubdivided; }
	void SetSomethingSubdivided(bool value) { _somethingWasSubdivided = value; }
	bool IsReset()
	{
		return _newTrianglesIDs.empty() && _newVerticesIDs.empty() && !_somethingWasMerged && !_somethingWasSubdivided;
	}
	void Reset()
	{
		_newTrianglesIDs.clear();
		_newVerticesIDs.clear();
		_somethingWasMerged = false;
		_somethingWasSubdivided = false;
	}

	enum TRI_EDGE_NUMBER: unsigned char
	{
		TRI_EDGE_1,	// index 0..1
		TRI_EDGE_2,	// index 1..2
		TRI_EDGE_3	// index 2..0
	};

	// Subdiv
	bool HandleTriangleSubdiv(unsigned int triIdx, bool testLength);
	bool HandleEdgeSubdiv(unsigned int triIdx, TRI_EDGE_NUMBER edgeNumber, bool testLength);	// return false if it's not possible

	// Merge
	bool RemoveCrushedTriangle(unsigned int triIdx, bool forceCompletelyFlatRemoval);
	void HandleEdgeMerge(unsigned int inputTriIdx, TRI_EDGE_NUMBER edgeNumber, bool doOnHardEdge);
	void StichLoops(LoopElement const* rootLoopA, unsigned int vtxToEliminateOnAIdx, LoopElement const* rootLoopB, unsigned int vtxToEliminateOnBIdx, unsigned int loopALength, unsigned int loopBLength, bool loopAreSameDir);
	float GetMinimumEdgeLengthSquared() const { return _dSquared; }

	// Thickness
	float GetThicknessSquared() const { return _thicknessSquared; }

private:
	// Subdiv
	void SubdivideTriangle(unsigned int triIdx, unsigned int vtx1, unsigned int vtx2, unsigned int newVtx);
	// Utility
	void ComputeTriNormal(unsigned int indices[3], Vector3& resultingNormal);

	Mesh& _mesh;
	std::vector<unsigned int>& _triangles;
	std::vector<Vector3>& _vertices;
	std::vector<Vector3> const& _trisNormal;
	std::vector<BSphere> const& _trisBSphere;
	std::vector<unsigned int> _newVerticesIDs;
	std::vector<unsigned int> _newTrianglesIDs;
	bool _somethingWasSubdivided;
	bool _somethingWasMerged;

	// Tessellation factors
	float _dDetail;	// Maximum edge length
	float _dDetailSquared;
	float _d;	// Minimum edge length
	float _dSquared;
	// Thickness factors
	float _thickness;
	float _thicknessSquared;
};

#endif // _RETESSELLATE_H_