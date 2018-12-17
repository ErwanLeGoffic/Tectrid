#include "ThicknessHandler.h"
#include "Mesh.h"
#include "Math\Math.h"
#include "Loop.h"
#include "OctreeVisitorGetIntersection.h"

//#define TEMP_CLEAN_SOLUTION	// Temporary solution which works with one stitch

ThicknessHandler::ThicknessHandler(Mesh& mesh): _mesh(mesh), _triangles(mesh.GrabTriangles()), _vertices(mesh.GrabVertices()), _normals(mesh.GetNormals())
{
	_verticesToTest.reserve(_mesh.GetVertices().size());
	_verticesMoved.reserve(_mesh.GetVertices().size());
}

void ThicknessHandler::ReshapeRegardingThickness(FUSION_MODE fusionMode)
{
	ASSERT(!_mesh.IsThereSomeVerticesWithTreatedFlag());
	Retessellate& retessellate = _mesh.GrabRetessellator();
	_distances.resize(_verticesMoved.size());
	bool somethingWasMerged = false;
	float thicknessSquared = retessellate.GetThicknessSquared();
	static float cosLimit = cosf(135.0f * float(M_PI) / 180.0f);
	for(unsigned int i = 0; i < _verticesMoved.size(); ++i)
	{
		unsigned int& vtxIdx = _verticesMoved[i];
		if(!_mesh.IsVertexToBeRemoved(vtxIdx) && !_mesh.IsVertexAlreadyTreated(vtxIdx))
		{
			Vector3 vtxNorm = _normals[vtxIdx];
			float& storedDist = _distances[i];
			storedDist = FLT_MAX;
			// Find closest point
			unsigned int closestPointVtxIdx = UNDEFINED_NEW_ID;
			for(unsigned int otherVtxIdx : _verticesToTest)
			{
				if((vtxIdx != otherVtxIdx) && !_mesh.IsVertexToBeRemoved(otherVtxIdx) && !_mesh.IsVertexAlreadyTreated(otherVtxIdx))
				{
					// Distance check
					float dist = (_vertices[otherVtxIdx] - _vertices[vtxIdx]).LengthSquared();
					if((dist < thicknessSquared) && (dist < storedDist))
					{
						// Orientation check
						Vector3 otherVtxNorm = _normals[otherVtxIdx];
						float dot = vtxNorm.Dot(otherVtxNorm);
						if(dot < cosLimit)
						{
							// Flag vertex and sourrdings in stiching zone to smooth those later
							_mesh.SetVertexInStitchingZone(vtxIdx);
							_mesh.SetVertexInStitchingZone(otherVtxIdx);
							for(unsigned int triIdx : _mesh.GetTrianglesAroundVertex(vtxIdx))
							{
								_mesh.SetVertexInStitchingZone(_triangles[triIdx * 3]);
								_mesh.SetVertexInStitchingZone(_triangles[triIdx * 3 + 1]);
								_mesh.SetVertexInStitchingZone(_triangles[triIdx * 3 + 2]);
							}
							for(unsigned int triIdx : _mesh.GetTrianglesAroundVertex(otherVtxIdx))
							{
								_mesh.SetVertexInStitchingZone(_triangles[triIdx * 3]);
								_mesh.SetVertexInStitchingZone(_triangles[triIdx * 3 + 1]);
								_mesh.SetVertexInStitchingZone(_triangles[triIdx * 3 + 2]);
							}
							// Adjascency check
							bool closestOppositePointIsAdjascent = false;
							std::vector<unsigned int> const& triAround = _mesh.GetTrianglesAroundVertex(otherVtxIdx);
							for(unsigned int triIdx : triAround)
							{
								unsigned int const* triVtxIdxs = &(_triangles[triIdx * 3]);
								if((triVtxIdxs[0] == vtxIdx) || (triVtxIdxs[1] == vtxIdx) || (triVtxIdxs[2] == vtxIdx))
								{
									closestOppositePointIsAdjascent = true;
									break;
								}
							}
							if(!closestOppositePointIsAdjascent)
							{
								storedDist = dist;
								closestPointVtxIdx = otherVtxIdx;
							}
						}
					}
				}
			}
#ifdef TEMP_CLEAN_SOLUTION
			/*if(somethingWasMerged)
				break;*/
			if((closestPointVtxIdx != UNDEFINED_NEW_ID) && (!somethingWasMerged))
#else
			if(closestPointVtxIdx != UNDEFINED_NEW_ID)
#endif // TEMP_CLEAN_SOLUTION
			{	// Treat fusion of the surfaces
				Vector3 delta = _vertices[closestPointVtxIdx] - _vertices[vtxIdx];
				bool vtxsAreFacing = delta.Dot(_normals[vtxIdx]) > 0.0f;
				FUSION_MODE fusionTypeToDo = FUSION_MODE_PIERCE;
				if(vtxsAreFacing)
					fusionTypeToDo = FUSION_MODE_MERGE;		// Have to merge
				else
					fusionTypeToDo = FUSION_MODE_PIERCE;	// Have to pierce
				if(fusionTypeToDo != fusionMode)
					continue;
				// Get loops : vertices ring around first and second vertex we will remove
				Loop ringLoopA = CollectRingLoop(vtxIdx);
				if(ringLoopA.IsEmpty() || !ringLoopA.IsClosed())
					continue;
				Loop ringLoopB = CollectRingLoop(closestPointVtxIdx);
				if(ringLoopB.IsEmpty() || !ringLoopB.IsClosed())
					continue;
				// Will B and A loop are in different direction ?
				bool loopAClockWize = ringLoopA.IsClockwize(_vertices[vtxIdx], _normals[vtxIdx], _vertices);
				bool loopBClockWize = ringLoopB.IsClockwize(_vertices[closestPointVtxIdx], _normals[closestPointVtxIdx], _vertices);
				bool loopAreSameDir = loopAClockWize != loopBClockWize;	// "!=" because the two loops normal are pointing in opposite direction
				// Stich loop A and B
				retessellate.StichLoops(ringLoopA.GetRoot(), vtxIdx, ringLoopB.GetRoot(), closestPointVtxIdx, ringLoopA.CountElements(), ringLoopB.CountElements(), loopAreSameDir);
				somethingWasMerged = true;
			}
		}
	}
	if(somethingWasMerged)
	{	// Smooth
		bool somethingMoved = false;
		unsigned int nbRepeat = 0;
		unsigned int nbGenVtxsBeforeAllLoops = (unsigned int) retessellate.GetGenratedVtxs().size();
		do
		{
			somethingMoved = false;
			if(fusionMode == FUSION_MODE_PIERCE)
			{
				for(unsigned int vtxIdx : _verticesToTest)
					somethingMoved |= SmoothVertex(vtxIdx);
			}
			else
			{
				for(unsigned int vtxIdx : _verticesToTest)
				{
					if(_mesh.IsVertexInStitchingZone(vtxIdx))
						somethingMoved |= SmoothVertex(vtxIdx);
				}
			}
			unsigned int nbGenVtxsBeforeLoop = (unsigned int) retessellate.GetGenratedVtxs().size();
			for(unsigned int i = nbGenVtxsBeforeAllLoops; i < nbGenVtxsBeforeLoop; ++i)
				somethingMoved |= SmoothVertex(retessellate.GetGenratedVtxs()[i]);
			nbRepeat++;
		} while((nbRepeat <= 2) && somethingMoved);
	}
	// Octree/normal/bbox/flags... update
	if((retessellate.GetGenratedVtxs().size() > 0) || (retessellate.GetGenratedTris().size() > 0))
	{
		_mesh.ReBalanceOctree(retessellate.GetGenratedTris(), retessellate.GetGenratedVtxs(), false);
		_mesh.RecomputeFragmentsBBox(false);
	}
	retessellate.Reset();
	_mesh.ClearAllVerticesInStitchingZoneAndAlreadyTreated();
}

Loop ThicknessHandler::CollectRingLoop(unsigned int vtxIdx)
{
	LoopBuilder loopBuilder(_mesh);
	std::vector<unsigned int> const& triAround = _mesh.GetTrianglesAroundVertex(vtxIdx);
	// Collect ring vertices
	//printf("Edge register\n");
	for(unsigned int triIdx : triAround)
	{
		unsigned int const* triVtxIdxs = &(_triangles[triIdx * 3]);
		std::vector<unsigned int> ringVtxIdx;
		ringVtxIdx.reserve(2);
		for(unsigned int i = 0; i < 3; ++i)
		{
			unsigned int idx = triVtxIdxs[i];
			if(idx != vtxIdx)
				ringVtxIdx.push_back(idx);
		}
		ASSERT(ringVtxIdx.size() == 2);
		if(ringVtxIdx.size() == 2)
		{
			if(!_mesh.IsVertexToBeRemoved(ringVtxIdx[0]) && !_mesh.IsVertexToBeRemoved(ringVtxIdx[1]))
				loopBuilder.RegisterEdge(ringVtxIdx[0], ringVtxIdx[1]);
		}
		//printf("%d - %d\n", ringVtxIdx[0], ringVtxIdx[1]);
	}
	std::vector<Loop> loops;
	loopBuilder.BuildLoops(loops);
	ASSERT(loops.size() == 1);

	/*if(!loops.empty())
	{
		printf("Loop:\n");
		LoopElement const* curVtx = loops[0].GetRoot();
		do
		{
			printf("%d\n", curVtx->GetVtxId());
			if(curVtx->GetNext() != nullptr)
				curVtx = curVtx->GetNext();
			else
				break;
		} while(curVtx != loops[0].GetRoot());
	}*/

	if(loops.size() == 1)
		return std::move(loops[0]);
	else
		return Loop(nullptr);
}

bool ThicknessHandler::SmoothVertex(unsigned int vtxIdx)
{
	bool somethingMoved = false;
	if(!_mesh.IsVertexToBeRemoved(vtxIdx)/* && _mesh.IsVertexInStitchingZone(vtxIdx)*/)
	{
		// Smooth
		Vector3 averageVertex;
		unsigned int nbVerticesInvolved = 0;
		for(unsigned int triIdx : _mesh.GetTrianglesAroundVertex(vtxIdx))
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
		if(nbVerticesInvolved > 0)
		{
			Vector3 newPos = averageVertex / float(nbVerticesInvolved);
			if((_vertices[vtxIdx] - newPos).LengthSquared() > 0.001f)
				somethingMoved = true;
			_vertices[vtxIdx] = newPos;
		}
		// Retesselate
		std::vector<unsigned int> triAround = _mesh.GetTrianglesAroundVertex(vtxIdx);
		for(unsigned int triIdx : triAround)
		{
			if(_mesh.IsTriangleToBeRemoved(triIdx))
				continue;
			_mesh.GrabRetessellator().RemoveCrushedTriangle(triIdx, true);
			if(_mesh.IsTriangleToBeRemoved(triIdx))
				continue;
			unsigned int const* vtxsIdx = &(_triangles[triIdx * 3]);
			unsigned int vtxId0 = vtxsIdx[0];
			unsigned int vtxId1 = vtxsIdx[1];
			unsigned int vtxId2 = vtxsIdx[2];
			Vector3 const& vtx0 = _vertices[vtxId0];
			Vector3 const& vtx1 = _vertices[vtxId1];
			Vector3 const& vtx2 = _vertices[vtxId2];
			// If needed, merge the smallest edge, subdivide the tallest.
			float squareLengthEdge1 = (vtx0 - vtx1).LengthSquared();
			float squareLengthEdge2 = (vtx1 - vtx2).LengthSquared();
			float squareLengthEdge3 = (vtx2 - vtx0).LengthSquared();
			if((squareLengthEdge1 < squareLengthEdge2) && (squareLengthEdge1 < squareLengthEdge3))
				_mesh.GrabRetessellator().HandleEdgeMerge(triIdx, Retessellate::TRI_EDGE_1, true);
			else if(squareLengthEdge2 < squareLengthEdge3)
				_mesh.GrabRetessellator().HandleEdgeMerge(triIdx, Retessellate::TRI_EDGE_2, true);
			else
				_mesh.GrabRetessellator().HandleEdgeMerge(triIdx, Retessellate::TRI_EDGE_3, true);
			if(_mesh.IsTriangleToBeRemoved(triIdx))
				continue;
			if((squareLengthEdge1 >= squareLengthEdge2) && (squareLengthEdge1 >= squareLengthEdge3))
				_mesh.GrabRetessellator().HandleEdgeSubdiv(triIdx, Retessellate::TRI_EDGE_1, true);
			else if(squareLengthEdge2 >= squareLengthEdge3)
				_mesh.GrabRetessellator().HandleEdgeSubdiv(triIdx, Retessellate::TRI_EDGE_2, true);
			else
				_mesh.GrabRetessellator().HandleEdgeSubdiv(triIdx, Retessellate::TRI_EDGE_3, true);
		}
	}
	return somethingMoved;
}