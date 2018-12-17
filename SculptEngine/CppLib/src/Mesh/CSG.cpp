#include "CSG.h"
#include "Mesh.h"
#include "Collisions\TriangleToTriangle.h"
#include "OctreeVisitorGetIntersection.h"
#include "OctreeVisitorRetessellateInRange.h"

bool Csg::DetectOctreeBBoxIntersection::HasToVisit(OctreeCell& cell)
{
	if(_collides)	// Once we detect a single collision, we get out.
		return false;
	if(cell.GetTrianglesIdx().size() > 0) 	// leaf cell
	{
		_collides = _collider.Intersects(cell.GetContentBBox());
		return !_collides;
	}
	return true;
}

bool Csg::VisitorGetOctreeBBoxIntersection::HasToVisit(OctreeCell& cell)
{
	return _collider.Intersects(cell.GetContentBBox());
}

void Csg::VisitorGetOctreeBBoxIntersection::VisitEnter(OctreeCell& cell)
{
	if(cell.GetTrianglesIdx().size() > 0) 	// leaf cell
		_collidedCells.push_back(&cell);
}

bool Csg::VisitorGatherCollidingCells::HasToVisit(OctreeCell& cell)
{
	if(cell.GetTrianglesIdx().size() > 0)	// leaf cell
	{
		Csg::VisitorGetOctreeBBoxIntersection _octreeBBoxIntersection(cell.GetContentBBox());
		_meshToCollideWith.GrabOctreeRoot().Traverse(_octreeBBoxIntersection);
		if(_octreeBBoxIntersection.GetCollidedCells().size() > 0)
			_colliderToCollidingCells[&cell] = _octreeBBoxIntersection.GetCollidedCells();
		return _octreeBBoxIntersection.GetCollidedCells().size() > 0;
	}
	else
	{
		Csg::DetectOctreeBBoxIntersection _octreeBBoxIntersection(cell.GetContentBBox());
		_meshToCollideWith.GrabOctreeRoot().Traverse(_octreeBBoxIntersection);
		return _octreeBBoxIntersection.Collides();
	}
}

bool Csg::ComputeCSGOperation(CSG_OPERATION operation, bool recenter)
{
	auto CloseFans = [](Mesh& mesh, std::vector<unsigned int>& triIdxVec)
	{
		std::set<unsigned int> triIdxSet(triIdxVec.begin(), triIdxVec.end());
		std::map<unsigned int, unsigned int> vtxIdToOccurenceCount;
		for(unsigned int triIdx: triIdxVec)
		{
			unsigned int const* vtxIdx = &(mesh.GetTriangles()[triIdx * 3]);
			for(unsigned int i = 0; i < 3; ++i)
			{
				if(vtxIdToOccurenceCount.find(vtxIdx[i]) == vtxIdToOccurenceCount.end())
					vtxIdToOccurenceCount[vtxIdx[i]] = 1;
				else
					++vtxIdToOccurenceCount[vtxIdx[i]];
			}
		}
		for(auto& pair : vtxIdToOccurenceCount)
		{
			if(pair.second > 3)
			{
				for(unsigned int triIdx : mesh.GetTrianglesAroundVertex(pair.first))
					triIdxSet.insert(triIdx);
			}
		}
		if(triIdxVec.size() < triIdxSet.size())
		{
			triIdxVec.clear();
			triIdxVec.insert(triIdxVec.end(), triIdxSet.begin(), triIdxSet.end());
		}
	};
	// Remove flat triangles from both meshes, also remove all pending removals, will be easier that way
	_firstMesh.RemoveFlatTriangles();
	_firstMesh.RecomputeNormals(true, false);
	_firstMesh.HandlePendingRemovals();
	_secondMesh.RemoveFlatTriangles();
	_secondMesh.RecomputeNormals(true, false);
	_secondMesh.HandlePendingRemovals();	

	// Grab colliding leaf octree cells
	VisitorGatherCollidingCells gatherCollidingCells(_secondMesh);
	_firstMesh.GrabOctreeRoot().Traverse(gatherCollidingCells);
	if(gatherCollidingCells.GetColliderToCollidingCells().empty())
	{
		// Build an output mesh
		_resultMesh.GrabVertices() = _firstMesh.GetVertices();
		_resultMesh.GrabTriangles() = _firstMesh.GetTriangles();
		_resultMesh.RebuildMeshData(false, recenter, true);
		return false;	// Objects doesn't collide, do nothing further
	}

	// Get triangles that collide together
	std::vector<unsigned int> intersectTrianglesFirstMesh;
	std::vector<unsigned int> intersectTrianglesSecondMesh;
	std::vector<Vector3> const& verticesFirstMesh = _firstMesh.GetVertices();
	std::vector<Vector3> const& verticesSecondMesh = _secondMesh.GetVertices();
	for(auto collidePair : gatherCollidingCells.GetColliderToCollidingCells())
	{
		std::vector<unsigned int> const& cellFirstMeshTriIdx = collidePair.first->GetTrianglesIdx();
		for(unsigned int triIdxFirst : cellFirstMeshTriIdx)
		{
			BSphere const& triSphereFirst = _firstMesh.GetTrisBSphere()[triIdxFirst];
			for(OctreeCell const* cell : collidePair.second)
			{
				std::vector<unsigned int> const& cellSecondMeshTriIdx = cell->GetTrianglesIdx();
				for(unsigned int triIdxSecond : cellSecondMeshTriIdx)
				{
					BSphere const& triSphereSecond = _secondMesh.GetTrisBSphere()[triIdxSecond];
					if(triSphereFirst.Intersects(triSphereSecond))
					{
						unsigned int const* vtxIdxFirst = &(_firstMesh.GetTriangles()[triIdxFirst * 3]);
						unsigned int const* vtxIdxSecond = &(_secondMesh.GetTriangles()[triIdxSecond * 3]);
						if(tri_tri_overlap_test_3d((float*) &verticesFirstMesh[vtxIdxFirst[0]], (float*) &verticesFirstMesh[vtxIdxFirst[1]], (float*) &verticesFirstMesh[vtxIdxFirst[2]],
							(float*) &verticesSecondMesh[vtxIdxSecond[0]], (float*) &verticesSecondMesh[vtxIdxSecond[1]], (float*) &verticesSecondMesh[vtxIdxSecond[2]]))
						{
							if(!_firstMesh.IsTriangleAlreadyTreated(triIdxFirst))
							{
								_firstMesh.SetTriangleAlreadyTreated(triIdxFirst);
								intersectTrianglesFirstMesh.push_back(triIdxFirst);
							}
							if(!_secondMesh.IsTriangleAlreadyTreated(triIdxSecond))
							{
								_secondMesh.SetTriangleAlreadyTreated(triIdxSecond);
								intersectTrianglesSecondMesh.push_back(triIdxSecond);
							}
#ifdef USE_BSP
							_firstMeshTriTouchesSecondMeshTris[triIdxFirst].push_back(triIdxSecond);
							_secondMeshTriTouchesFirstMeshTris[triIdxSecond].push_back(triIdxFirst);
#endif // USE_BSP
						}
					}
				}
			}
		}
	}
	if(intersectTrianglesFirstMesh.empty())
	{
		ASSERT(intersectTrianglesSecondMesh.empty());
		// Build an output mesh
		_resultMesh.GrabVertices() = _firstMesh.GetVertices();
		_resultMesh.GrabTriangles() = _firstMesh.GetTriangles();
		_resultMesh.RebuildMeshData(false, recenter, true);
		return false;	// Objects doesn't collide, do nothing further
	}
	_firstMesh.ClearAllTrianglesAlreadyTreated();
	_secondMesh.ClearAllTrianglesAlreadyTreated();

	// Close fans
	for(auto& pair : _firstMeshTriTouchesSecondMeshTris)
	{
		if(pair.second.size() > 2)
			CloseFans(_secondMesh, pair.second);
	}
	for(auto& pair : _secondMeshTriTouchesFirstMeshTris)
	{
		if(pair.second.size() > 2)
			CloseFans(_firstMesh, pair.second);
	}

	// Invalidate inside mesh parts
	if(operation == INTERSECT)
	{
		FlipMesh(_secondMesh);
	}
	InvalidateInsideMeshPart(_firstMesh, _secondMesh, intersectTrianglesFirstMesh);
	if(operation == INTERSECT)
	{
		FlipMesh(_secondMesh);
		FlipMesh(_firstMesh);
	}
	if(operation == SUBTRACT)
	{
		FlipMesh(_firstMesh);
		FlipMesh(_secondMesh);
	}
	InvalidateInsideMeshPart(_secondMesh, _firstMesh, intersectTrianglesSecondMesh);
	if(operation == INTERSECT)
	{
		FlipMesh(_firstMesh);
	}
	if(operation == SUBTRACT)
	{
		FlipMesh(_firstMesh);
	}

#ifdef USE_BSP
	if(operation == MERGE)
	{
		FlipMesh(_secondMesh);
	}
	for(auto& pair : _firstMeshTriTouchesSecondMeshTris)
	{
		Csg_Bsp first = BuildCSGBsp(std::vector<unsigned int>(1, pair.first), _firstMesh);
		Csg_Bsp second = BuildCSGBsp(pair.second, _secondMesh);
		AddToCsgResult(first.Subtract(second, false, SculptEngine::IsTriangleOrientationInverted() ? false : true));
	}
	if(operation == MERGE)
	{
		FlipMesh(_secondMesh);
		FlipMesh(_firstMesh);
	}
	for(auto& pair : _secondMeshTriTouchesFirstMeshTris)
	{
		Csg_Bsp first = BuildCSGBsp(std::vector<unsigned int>(1, pair.first), _secondMesh);
		Csg_Bsp second = BuildCSGBsp(pair.second, _firstMesh);
		AddToCsgResult(first.Subtract(second, false, SculptEngine::IsTriangleOrientationInverted() ? false : true));
	}
	if(operation == MERGE)
	{
		FlipMesh(_firstMesh);
	}

#ifdef DO_STITCH_INTERSECTION_ZONE
	StitchIntersectionZone();
#endif // DO_STITCH_INTERSECTION_ZONE

	_resultTriangles = _csgBspTrianglesResult;
	_resultVertices.empty();
	_resultVertices.reserve(_csgBspVerticesResult.size());
	for(Vector3Double& vertex : _csgBspVerticesResult)
		_resultVertices.push_back(Vector3(float (vertex.x), float(vertex.y), float(vertex.z)));

#else
	// Clean mesh (pending to remove geometry)
	/*_firstMesh.HandlePendingRemovals(nullptr);
	_secondMesh.HandlePendingRemovals(nullptr);*/
	
	{
		// Build loops
		LoopBuilder firstMeshLoopBuilder(_firstMesh);
		_firstMesh.TagAndCollectOpenEdgesVertices(&firstMeshLoopBuilder);
		firstMeshLoopBuilder.BuildLoops(_firstMeshLoops);

		LoopBuilder secondMeshLoopBuilder(_secondMesh);
		_secondMesh.TagAndCollectOpenEdgesVertices(&secondMeshLoopBuilder);
		secondMeshLoopBuilder.BuildLoops(_secondMeshLoops);
	}

	// Something can go wrong when there are very thin zones in the objects. In that case we will end up with a different number of loops between the two meshes... For the moment the solution is: flee!
	if(_firstMeshLoops.size() != _secondMeshLoops.size())
	{
		_firstMesh.Undo();
		return false;
	}

	for(int i = 0; i < 5; i++)
	{
		for(Loop& loopFirstMesh : _firstMeshLoops)
		{
			VisitorRetessellateInLoopRange retessellate(_firstMesh, loopFirstMesh, 10.0f, 2.0f);
			_firstMesh.GrabOctreeRoot().Traverse(retessellate);
		}

		for(Loop& loopSecondMesh : _secondMeshLoops)
		{
			VisitorRetessellateInLoopRange retessellate(_secondMesh, loopSecondMesh, 10.0f, 2.0f);
			_secondMesh.GrabOctreeRoot().Traverse(retessellate);
		}

		// build loops again
		_firstMeshLoops.clear();
		_secondMeshLoops.clear();
		LoopBuilder firstMeshLoopBuilder2(_firstMesh);
		_firstMesh.TagAndCollectOpenEdgesVertices(&firstMeshLoopBuilder2);
		firstMeshLoopBuilder2.BuildLoops(_firstMeshLoops);

		LoopBuilder secondMeshLoopBuilder2(_secondMesh);
		_secondMesh.TagAndCollectOpenEdgesVertices(&secondMeshLoopBuilder2);
		secondMeshLoopBuilder2.BuildLoops(_secondMeshLoops);

		PairLoops();

		StitchProgress();
	}

	StitchFinalize();
#endif //!USE_BSP

	// Build resulting mesh
	RebuildMeshFromCsg(recenter);

	return true;
}

void Csg::ExtractRemainingMeshTess(Mesh& inMesh, std::vector<Vector3>& outVertices, std::vector<unsigned int>& outTriangles)
{
	// Find triangles to extract
	std::vector<unsigned int> selectedTriFirstMesh;
	selectedTriFirstMesh.reserve(inMesh.GetTrisNormal().size());
	for(unsigned int i = 0; i < inMesh.GetTrisNormal().size(); ++i)
	{
		if(inMesh.IsTriangleToBeRemoved(i) == false)
			selectedTriFirstMesh.push_back(i);
	}

	// Build the list of needed unique vertex indices
	std::vector<unsigned int> const& trianglesFullmesh = inMesh.GetTriangles();
	std::vector<Vector3> const& verticesFullmesh = inMesh.GetVertices();
	std::vector<int> reqVtxIndices;
	reqVtxIndices.reserve(selectedTriFirstMesh.size() * 3);	// Of course we reserve too much memory (this would be the size we would get if no triangles were weld together), but that's worth it as we don't know the exact size we will need and to avoid to have to reallocate after
	for(unsigned int const& triIdx : selectedTriFirstMesh)
	{
		unsigned int const* vtxsIdx = &(trianglesFullmesh[triIdx * 3]);
		for(int i = 0; i < 3; ++i)
		{
			if(!inMesh.IsVertexAlreadyTreated(vtxsIdx[i]))
			{
				inMesh.SetVertexAlreadyTreated(vtxsIdx[i]);
				reqVtxIndices.push_back(vtxsIdx[i]);
			}
		}
	}
	// Build vertex vector from collected unique vertex indices
	outVertices.reserve(reqVtxIndices.size());
	std::map<int, int> vtxIdxRemap;
	for(int const& vtxIdx : reqVtxIndices)
	{
		vtxIdxRemap[vtxIdx] = (int) outVertices.size();
		outVertices.push_back(verticesFullmesh[vtxIdx]);
		inMesh.ClearVertexAlreadyTreated(vtxIdx);
	}
	// Build triangles draw list
	outTriangles.reserve(selectedTriFirstMesh.size() * 3);
	for(unsigned int const& triIdx : selectedTriFirstMesh)
	{
		unsigned int const* vtxsIdx = &(trianglesFullmesh[triIdx * 3]);
		for(int i = 0; i < 3; ++i)
			outTriangles.push_back(vtxIdxRemap[vtxsIdx[i]]);
	}
}

void Csg::FlipMesh(Mesh& mesh)
{
	std::vector<unsigned int>& tris = mesh.GrabTriangles();
	for(unsigned int i = 0; i < tris.size(); i += 3)
	{
		unsigned int temp = tris[i];
		tris[i] = tris[i + 2];
		tris[i + 2] = temp;
	}
	for(Vector3& normal : mesh.GrabTrisNormal())
		normal.Negate();
	for(Vector3& normal : mesh.GrabNormals())
		normal.Negate();
}

void Csg::InvalidateInsideMeshPart(Mesh& mesh, Mesh& otherMesh, std::vector<unsigned int> const& intersectionTriangles)
{
	for(unsigned int triIdx : intersectionTriangles)
	{
		// Remove connectivity with intersection triangles
		unsigned int const* vtxIdxs = &(mesh.GetTriangles()[triIdx * 3]);
		for(unsigned int i = 0; i < 3; ++i)
		{
			if(mesh.GetTrianglesAroundVertex(vtxIdxs[i]).size() == 1)
				mesh.SetVertexToBeRemoved(vtxIdxs[i]);
			mesh.RemoveTriangleAroundVertex(vtxIdxs[i], triIdx);
		}
	}
	for(unsigned int triIdx : intersectionTriangles)
	{
		if(!mesh.IsTriangleToBeRemoved(triIdx))
		{
			auto RayCastAndInvalidate = [&](unsigned int vtxIdxA, unsigned int vtxIdxB) -> bool
			{
				if(mesh.IsVertexAlreadyTreated(vtxIdxA) || mesh.IsVertexAlreadyTreated(vtxIdxB))
					return false;
				Vector3 const& a = mesh.GetVertices()[vtxIdxA];
				Vector3 const& b = mesh.GetVertices()[vtxIdxB];
				Vector3 edge = b - a;
				float edgeLength = edge.Length();
				ASSERT(edgeLength > 0.0f);
				if(edgeLength > EPSILON)
				{
					Ray ray(a, edge / edgeLength, edgeLength);
					VisitorGetIntersection getIntersection(otherMesh, ray, false, true);
					otherMesh.GrabOctreeRoot().Traverse(getIntersection);
					if(getIntersection.GetIntersectionDists().size() == 1)	// 0: no col, >1: the ray traversed the mesh
					{
						unsigned int vtxIdxInside;
						if(edge.Dot(getIntersection.GetIntersectionNormal(0)) > 0.0f)
							vtxIdxInside = vtxIdxA;	// a is inside the other mesh 
						else
							vtxIdxInside = vtxIdxB;	// b is inside the other mesh 
																				// Invalidate all triangles up to the mesh edge limit (possible because we remove intersection triangle connectivity just before)
						std::vector<unsigned int> verticesToInvalidate;
						if(!mesh.IsVertexAlreadyTreated(vtxIdxInside))
						{
							{	// Before we go any further, let's double check: Sometimes, due to floating point issues, we get incorrect result (for the moment, seen only on the web browser)
								// So we cast on the opposite way, just to be sure
								Vector3 edgeCheck = a - b;
								float edgeCheckLength = edgeCheck.Length();
								Ray rayCheck(b, edgeCheck / edgeCheckLength, edgeCheckLength);
								VisitorGetIntersection getIntersectionCheck(otherMesh, rayCheck, false, true);
								otherMesh.GrabOctreeRoot().Traverse(getIntersectionCheck);
								if(getIntersectionCheck.GetIntersectionDists().size() != 1)
								{	// Check failed
									ASSERT(false);
									return false;
								}
								if(getIntersection.GetIntersectionNormal(0) != getIntersectionCheck.GetIntersectionNormal(0))
								{	// Check failed
									ASSERT(false);
									return false;
								}
							}
							verticesToInvalidate.push_back(vtxIdxInside);
							mesh.SetVertexAlreadyTreated(verticesToInvalidate.back());
						}
						while(verticesToInvalidate.empty() == false)
						{
							unsigned int vtxIdxToInvalidate = verticesToInvalidate.back();
							verticesToInvalidate.pop_back();
							std::vector<unsigned int> const& trisToInvalidate = mesh.GetTrianglesAroundVertex(vtxIdxToInvalidate);
							for(unsigned int triIdxToInvalidate : trisToInvalidate)
							{
								if(!mesh.IsTriangleToBeRemoved(triIdxToInvalidate))
								{
									mesh.SetTriangleToBeRemoved(triIdxToInvalidate);
									unsigned int const* vtxIdxs = &(mesh.GetTriangles()[triIdxToInvalidate * 3]);
									for(unsigned int i = 0; i < 3; ++i)
									{
										if(!mesh.IsVertexAlreadyTreated(vtxIdxs[i]))
										{
											verticesToInvalidate.push_back(vtxIdxs[i]);
											mesh.SetVertexAlreadyTreated(vtxIdxs[i]);
										}
									}
								}
							}
						}
						return true;
					}
				}
				return false;
			};
			
			{
				unsigned int const* vtxIdxs = &(mesh.GetTriangles()[triIdx * 3]);
				if(!RayCastAndInvalidate(vtxIdxs[0], vtxIdxs[1]))
				{
					/*if(!RayCastAndInvalidate(vtxIdxs[1], vtxIdxs[2]))
						RayCastAndInvalidate(vtxIdxs[2], vtxIdxs[0]);*/
				}
			}
			mesh.SetTriangleToBeRemoved(triIdx);
		}		
	}
	mesh.ClearAllVerticesAlreadyTreated();
}

#ifdef USE_BSP

Csg_Bsp Csg::BuildCSGBsp(std::vector<unsigned int> trianglesIdx, Mesh& mesh)
{
	std::vector<Csg_Bsp::Polygon> polys;
	polys.reserve(trianglesIdx.size());
	std::vector<Csg_Bsp::Vertex> polyVertices;
	polyVertices.reserve(3);
	std::vector<unsigned int> const& triangles = mesh.GetTriangles();
	std::vector<Vector3> const& vertices = mesh.GetVertices();
	for(unsigned int triIdx : trianglesIdx)
	{
//	for(signed int i = (signed int) trianglesIdx.size() - 1; i >= 0; i--)
//	{
//		unsigned int triIdx = trianglesIdx[i];
		unsigned int const* vtxIdx = &(triangles[triIdx * 3]);
		polyVertices.push_back(Csg_Bsp::Vertex(vertices[*vtxIdx])); vtxIdx++;
		polyVertices.push_back(Csg_Bsp::Vertex(vertices[*vtxIdx])); vtxIdx++;
		polyVertices.push_back(Csg_Bsp::Vertex(vertices[*vtxIdx]));
		polys.push_back(Csg_Bsp::Polygon(polyVertices));
		polyVertices.clear();
	}
	return Csg_Bsp(polys, true);
}

void Csg::AddToCsgResult(Csg_Bsp elementToAdd)
{
	auto IsCrushedTriangle = [&](Vector3Double const& vtx0, Vector3Double const& vtx1, Vector3Double const& vtx2) -> bool
	{	// Remove triangles that are crushed: like one of the vertex get close to its opposite edge
		static const double crushLengthThreshold = sqr(EPSILON_DBL);
		Vector3Double const edge1(vtx1 - vtx0);
		Vector3Double const edge2(vtx2 - vtx1);
		Vector3Double const edge3(vtx0 - vtx2);
		// will find the tallest edge
		double squaredLengthEdge1 = edge1.LengthSquared();
		double squaredLengthEdge2 = edge2.LengthSquared();
		double squaredLengthEdge3 = edge3.LengthSquared();
		if((squaredLengthEdge1 < crushLengthThreshold) || (squaredLengthEdge2 < crushLengthThreshold) || (squaredLengthEdge3 < crushLengthThreshold))
		{	// Flat edge triangle
			return true;
		}
		Vector3Double const* tallestEdge;
		Vector3Double const* tallestEdgeStart;
		Vector3Double const* vertexOpositeToTallestEdge;
		double squaredLengthTallestEdge;
		if((squaredLengthEdge1 >= squaredLengthEdge2) && (squaredLengthEdge1 >= squaredLengthEdge3))
		{
			tallestEdge = &edge1;
			tallestEdgeStart = &vtx0;
			vertexOpositeToTallestEdge = &vtx2;
			squaredLengthTallestEdge = squaredLengthEdge1;
		}
		else if(squaredLengthEdge2 >= squaredLengthEdge3)
		{
			tallestEdge = &edge2;
			tallestEdgeStart = &vtx1;
			vertexOpositeToTallestEdge = &vtx0;
			squaredLengthTallestEdge = squaredLengthEdge2;
		}
		else
		{
			tallestEdge = &edge3;
			tallestEdgeStart = &vtx2;
			vertexOpositeToTallestEdge = &vtx1;
			squaredLengthTallestEdge = squaredLengthEdge3;
		}
		double lengthTallestEdge = sqrt(squaredLengthTallestEdge);
		Vector3Double const tallestEdgeDir = *tallestEdge / lengthTallestEdge;
		double dot = tallestEdgeDir.Dot(*vertexOpositeToTallestEdge - *tallestEdgeStart);
		ASSERT((dot > 0.0) && (dot < lengthTallestEdge));
		Vector3Double const projectedPoint = *tallestEdgeStart + tallestEdgeDir * dot;
		// Get distance from opposite vertex to its projection on tallest edge, this is what we take in account to determine if the triangle is crushed or not
		double distSquaredOpositeToProjected = (*vertexOpositeToTallestEdge - projectedPoint).LengthSquared();
		if(distSquaredOpositeToProjected < crushLengthThreshold)
		{	// Triangle crushed
			return true;
		}
		return false;
	};
	for(Csg_Bsp::Polygon polygon : elementToAdd.GetPolygons())
	{
		Vector3Double vtx1(polygon.GetVertices()[0].GetPos());
		for(unsigned int i = 1; i < (unsigned int) polygon.GetVertices().size() - 1; ++i)
		{
			Vector3Double vtx2(polygon.GetVertices()[i].GetPos());
			Vector3Double vtx3(polygon.GetVertices()[i + 1].GetPos());

			if(!IsCrushedTriangle(vtx1, vtx2, vtx3))
			{
				_csgBspTrianglesResult.push_back((unsigned int) _csgBspVerticesResult.size());
				_csgBspVerticesResult.push_back(vtx1);
#ifdef DO_STITCH_INTERSECTION_ZONE
				unsigned int triIdx = (unsigned int) _csgBspTrianglesResult.size() / 3;
				_csgBspVtxToTriAround.push_back({ triIdx });
				_csgBspVtxsToBeRemoved.push_back(false);
#endif // DO_STITCH_INTERSECTION_ZONE

				_csgBspTrianglesResult.push_back((unsigned int) _csgBspVerticesResult.size());
				_csgBspVerticesResult.push_back(vtx2);
#ifdef DO_STITCH_INTERSECTION_ZONE
				_csgBspVtxToTriAround.push_back({ triIdx });
				_csgBspVtxsToBeRemoved.push_back(false);
#endif // DO_STITCH_INTERSECTION_ZONE

				_csgBspTrianglesResult.push_back((unsigned int) _csgBspVerticesResult.size());
				_csgBspVerticesResult.push_back(vtx3);
#ifdef DO_STITCH_INTERSECTION_ZONE
				_csgBspVtxToTriAround.push_back({ triIdx });
				_csgBspVtxsToBeRemoved.push_back(false);

				_csgBspTrianglesBSphere.push_back(BSphereDouble(vtx1, vtx2, vtx3));
#endif // DO_STITCH_INTERSECTION_ZONE
			}
		}
	}
}

#ifdef DO_STITCH_INTERSECTION_ZONE
void Csg::CheckVertexIsCorrect(unsigned int vtxIdx)
{
	for(unsigned int const& triIdx : _csgBspVtxToTriAround[vtxIdx])
	{
		ASSERT(triIdx < (_csgBspTrianglesResult.size() / 3));
		if(CsgBspIsVertexToBeRemoved(triIdx))
			continue;
		unsigned int const* vtxsIdx = &(_csgBspTrianglesResult[triIdx * 3]);
		// *** edge shared by 2 triangles only check ****
		bool found = false;
		for(int i = 0; i < 3; ++i)
		{
			if(vtxsIdx[i] == vtxIdx)
				found = true;
			else
			{	// Test that an edge is only shared by two triangles
				std::vector<unsigned int> const& triAroundA = _csgBspVtxToTriAround[vtxIdx];
				std::vector<unsigned int> const& triAroundB = _csgBspVtxToTriAround[vtxsIdx[i]];
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
				ASSERT(nbSharedTriangles <= 2);
			}
		}
		ASSERT(found);
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

void Csg::StitchIntersectionZone()
{
	struct BestEdgeResult
	{
		double distance;
		Vector3Double projPointOnSegment;
		unsigned int triId;
		unsigned int vtxIDA;
		unsigned int vtxIDB;
		unsigned int vtxIDOther;
	} bestEdgeResult;
	struct BestVertexResult
	{
		double distance;
		unsigned int vtxID;
	} bestVertexResult;
	auto CheckVertexIsClosed = [&](unsigned int vtxIdx) -> bool	// Returns true if the vertex is already completely surrounded by the triangles it's linked to
	{
		for(unsigned int const& triIdx : _csgBspVtxToTriAround[vtxIdx])
		{
			unsigned int const* vtxsIdx = &(_csgBspTrianglesResult[triIdx * 3]);
			// *** edge shared by 2 triangles only check ****
			for(int i = 0; i < 3; ++i)
			{
				if(vtxsIdx[i] != vtxIdx)
				{	// Test that an edge is only shared by two triangles
					std::vector<unsigned int> const& triAroundA = _csgBspVtxToTriAround[vtxIdx];
					std::vector<unsigned int> const& triAroundB = _csgBspVtxToTriAround[vtxsIdx[i]];
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
	};
	auto GetVertexDist = [&](unsigned int vtxToTreatIdx, unsigned int vtxIdxToTestOnto) -> double
	{
		return (_csgBspVerticesResult[vtxToTreatIdx] - _csgBspVerticesResult[vtxIdxToTestOnto]).Length();
	};
	auto MergeVertex = [&](unsigned int vtxToTreatIdx, unsigned int vtxIdxToMergeOnto)
	{
		CsgBspSetVertexToBeRemoved(vtxToTreatIdx);
		for(unsigned int triIdxToRemapVtx : _csgBspVtxToTriAround[vtxToTreatIdx])
		{
			unsigned int* triVtxIdx = &(_csgBspTrianglesResult[triIdxToRemapVtx * 3]);
			for(unsigned int k = 0; k < 3; ++k)
			{
				if(triVtxIdx[k] == vtxToTreatIdx)
				{
					triVtxIdx[k] = vtxIdxToMergeOnto;
					CsgBspAddTriangleAroundVertex(vtxIdxToMergeOnto, triIdxToRemapVtx);
					break;
				}
			}
		}
		CsgBspClearTriangleAroundVertex(vtxToTreatIdx);
	};
	auto TestRelyOnEdge = [&](unsigned int vtxToTreatIdx, unsigned int vtxIdxA, unsigned int vtxIdxB, Vector3Double& projPointOnSegment) -> double	// Return the distance from vtxToTreatIdx to A-B segment if they are pointing in the same direction and if A-vtxToTreatIdx is smaller than A-B (returns DBG_MAX instead)
	{
		std::vector<Vector3Double> const& vertices = _csgBspVerticesResult;
		Vector3Double const& A = vertices[vtxIdxA];
		Vector3Double const& B = vertices[vtxIdxB];
		Vector3Double const& Other = vertices[vtxToTreatIdx];
		Vector3Double AToB = B - A;
		Vector3Double AToOther = Other - A;
		double lengthAToB = AToB.Length();
		double lengthAToOther = AToOther.Length();
		//ASSERT(lengthAToOther != 0.0);
		if(lengthAToOther >= lengthAToB)
			return DBL_MAX;
		if(lengthAToB == 0.0)
			return DBL_MAX;
		Vector3Double AToBDir = AToB / lengthAToB;
		double dot = AToBDir.Dot(AToOther);
		if(dot < 0.0)
			return DBL_MAX;
		projPointOnSegment = (AToBDir * dot) + A;
		return (Other - projPointOnSegment).Length();
	};
	auto TestTriangles = [&](unsigned int vtxToTreatIdx)	// Will update bestVertexResult and bestEdgeResult
	{
		unsigned int nbTris = (unsigned int) _csgBspTrianglesResult.size() / 3;
		for(unsigned int triIdx = 0; (bestVertexResult.distance != 0.0f) && (bestEdgeResult.distance != 0.0f) && (triIdx < nbTris); ++triIdx)
		{
			BSphereDouble const& triBSphere = _csgBspTrianglesBSphere[triIdx];
			if(triBSphere.Intersects(_csgBspVerticesResult[vtxToTreatIdx], EPSILON_DBL))		// Check for each vertex if it touches a triangle b-sphere
			{	// Intersects
				// Test if that triangle doesn't use that vertex already
				unsigned int* triVtxIdxs = &(_csgBspTrianglesResult[triIdx * 3]);
				if((triVtxIdxs[0] != vtxToTreatIdx) && (triVtxIdxs[1] != vtxToTreatIdx) && (triVtxIdxs[2] != vtxToTreatIdx))
				{	// If that's not the case, then test if the vertex rely on one of the triangle edge
					double distVertex = GetVertexDist(vtxToTreatIdx, triVtxIdxs[0]);
					if(distVertex < bestVertexResult.distance)
					{
						bestVertexResult.distance = distVertex;
						bestVertexResult.vtxID = triVtxIdxs[0];
					}
					distVertex = GetVertexDist(vtxToTreatIdx, triVtxIdxs[1]);
					if(distVertex < bestVertexResult.distance)
					{
						bestVertexResult.distance = distVertex;
						bestVertexResult.vtxID = triVtxIdxs[1];
					}
					distVertex = GetVertexDist(vtxToTreatIdx, triVtxIdxs[2]);
					if(distVertex < bestVertexResult.distance)
					{
						bestVertexResult.distance = distVertex;
						bestVertexResult.vtxID = triVtxIdxs[2];
					}
					double dists[3];
					Vector3Double projPointsOnSegment[3];
					dists[0] = TestRelyOnEdge(vtxToTreatIdx, triVtxIdxs[0], triVtxIdxs[1], projPointsOnSegment[0]);
					dists[1] = TestRelyOnEdge(vtxToTreatIdx, triVtxIdxs[1], triVtxIdxs[2], projPointsOnSegment[1]);
					dists[2] = TestRelyOnEdge(vtxToTreatIdx, triVtxIdxs[2], triVtxIdxs[0], projPointsOnSegment[2]);
					double minDist = min(min(dists[0], dists[1]), dists[2]);	// On very flat triangles, cosine could approx 1.0 on more than one edge
					if(minDist < bestEdgeResult.distance)
					{
						bestEdgeResult.distance = minDist;
						bestEdgeResult.triId = triIdx;
						if((dists[0] <= dists[1]) && (dists[0] <= dists[2]))
						{
							bestEdgeResult.vtxIDA = 0;
							bestEdgeResult.vtxIDB = 1;
							bestEdgeResult.vtxIDOther = 2;
							bestEdgeResult.projPointOnSegment = projPointsOnSegment[0];
						}
						else if((dists[1] <= dists[0]) && (dists[1] <= dists[2]))
						{
							bestEdgeResult.vtxIDA = 1;
							bestEdgeResult.vtxIDB = 2;
							bestEdgeResult.vtxIDOther = 0;
							bestEdgeResult.projPointOnSegment = projPointsOnSegment[1];
						}
						else // if((dists[2] <= dists[1]) && (dists[2] <= dists[0]))
						{
							bestEdgeResult.vtxIDA = 2;
							bestEdgeResult.vtxIDB = 0;
							bestEdgeResult.vtxIDOther = 1;
							bestEdgeResult.projPointOnSegment = projPointsOnSegment[2];
						}
					}
				}
			}
		}
	};

	// *** Will subdivide triangle to link with vertices that lies on the edge of another triangle ***
	// Merge CSG generated vertices
	// For each CSG created vertex we will try to find a triangle with an edge clearly touching
	//std::vector<unsigned int> unStitchedVtxIds;
	bool mergeOccured = false;
	do
	{
		mergeOccured = false;
		for(unsigned int vtxToTreatIdx = 0; vtxToTreatIdx < _csgBspVerticesResult.size(); ++vtxToTreatIdx)
		{
			if(CsgBspIsVertexToBeRemoved(vtxToTreatIdx))
				continue;
			bestEdgeResult.distance = DBL_MAX;
			bestEdgeResult.triId = UNDEFINED_NEW_ID;
			bestVertexResult.distance = DBL_MAX;
			bestVertexResult.vtxID = UNDEFINED_NEW_ID;
			// Scan for closest
			TestTriangles(vtxToTreatIdx);
			// Vertex merge
			if(bestVertexResult.distance < EPSILON_DBL * EPSILON_DBL)
			{
#ifdef MESH_CONSISTENCY_CHECK
				CheckVertexIsCorrect(bestVertexResult.vtxID);
#endif // MESH_CONSISTENCY_CHECK
				MergeVertex(vtxToTreatIdx, bestVertexResult.vtxID);
				mergeOccured = true;
#ifdef MESH_CONSISTENCY_CHECK
				CheckVertexIsCorrect(bestVertexResult.vtxID);
#endif // MESH_CONSISTENCY_CHECK
			}
			// Edge split
			else if(bestEdgeResult.triId != UNDEFINED_NEW_ID)
			{
				//ASSERT(bestResult.distance < EPSILON_DBL);
				if(CheckVertexIsClosed(vtxToTreatIdx))
					continue;
				if(bestEdgeResult.distance >= EPSILON_DBL * EPSILON_DBL)
				{
					//unStitchedVtxIds.push_back(vtxToTreatIdx);
					continue;		// Could happen if the vertex is already completely surrounded by the triangles it's linked to
				}
				unsigned int* triVtxIdxs = &(_csgBspTrianglesResult[bestEdgeResult.triId * 3]);
#ifdef MESH_CONSISTENCY_CHECK
				CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDA]);
				CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDB]);
				CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDOther]);
				CheckVertexIsCorrect(vtxToTreatIdx);
#endif // MESH_CONSISTENCY_CHECK
				// *** snap on the segment the vertex we are treating 
				//if((_csgBspVerticesResult[vtxToTreatIdx] - bestEdgeResult.projPointOnSegment).Length() >= EPSILON_DBL)
				//	__debugbreak();
				//_csgBspVerticesResult[vtxToTreatIdx] = bestEdgeResult.projPointOnSegment;
				// *** First triangle ***
				unsigned int vtxIdxBBackup = triVtxIdxs[bestEdgeResult.vtxIDB];
				// Modify one vertex (the "B" one) in the current triangle. Getting a triangle with A - vtxToTreatIdx - vtxIDOther
				triVtxIdxs[bestEdgeResult.vtxIDB] = vtxToTreatIdx;
				// Force b-sphere rebuild
				_csgBspTrianglesBSphere[bestEdgeResult.triId].BuildFromTriangle(_csgBspVerticesResult[triVtxIdxs[bestEdgeResult.vtxIDA]], _csgBspVerticesResult[triVtxIdxs[bestEdgeResult.vtxIDB]], _csgBspVerticesResult[triVtxIdxs[bestEdgeResult.vtxIDOther]]);
				// Update triangle around vertex data
				CsgBspAddTriangleAroundVertex(triVtxIdxs[bestEdgeResult.vtxIDB], bestEdgeResult.triId);
				// *** Second triangle ***
				// Create a second triangle using vtxToTreatIdx - B - vtxIDOther
				// Take care: _triangles might be reallocated, all pointers on its data will be invalid
				unsigned int newTriIdx = CsgBspAddTriangle(vtxToTreatIdx, vtxIdxBBackup, triVtxIdxs[bestEdgeResult.vtxIDOther]);
				triVtxIdxs = &(_csgBspTrianglesResult[bestEdgeResult.triId * 3]);
				// Update triangle around vertex data
				CsgBspChangeTriangleAroundVertex(vtxIdxBBackup, bestEdgeResult.triId, newTriIdx);
				CsgBspAddTriangleAroundVertex(vtxToTreatIdx, newTriIdx);
				CsgBspAddTriangleAroundVertex(triVtxIdxs[bestEdgeResult.vtxIDOther], newTriIdx);
#ifdef MESH_CONSISTENCY_CHECK
				CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDA]);
				CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDB]);
				CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDOther]);
				CheckVertexIsCorrect(vtxToTreatIdx);
#endif // MESH_CONSISTENCY_CHECK
			}
		}
	} while(mergeOccured);
	/*for(unsigned int vtxToTreatIdx = 0; vtxToTreatIdx < _csgBspVerticesResult.size(); ++vtxToTreatIdx)
	{
		if(CsgBspIsVertexToBeRemoved(vtxToTreatIdx))
			continue;
		bestEdgeResult.distance = DBL_MAX;
		bestEdgeResult.triId = UNDEFINED_NEW_ID;
		bestVertexResult.distance = DBL_MAX;
		bestVertexResult.vtxID = UNDEFINED_NEW_ID;
		// Scan for closest
		TestTriangles(vtxToTreatIdx);
		// Edge split
		if(bestEdgeResult.triId != UNDEFINED_NEW_ID)
		{
			//ASSERT(bestResult.distance < EPSILON_DBL);
			if(CheckVertexIsClosed(vtxToTreatIdx))
				continue;
			if(bestEdgeResult.distance >= EPSILON_DBL)
			{
				unStitchedVtxIds.push_back(vtxToTreatIdx);
				continue;		// Could happen if the vertex is already completely surrounded by the triangles it's linked to
			}
			unsigned int* triVtxIdxs = &(_csgBspTrianglesResult[bestEdgeResult.triId * 3]);
#ifdef MESH_CONSISTENCY_CHECK
			CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDA]);
			CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDB]);
			CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDOther]);
			CheckVertexIsCorrect(vtxToTreatIdx);
#endif // MESH_CONSISTENCY_CHECK
			// *** snap on the segment the vertex we are treating 
			//_csgBspVerticesResult[vtxToTreatIdx] = bestEdgeResult.projPointOnSegment;
			// *** First triangle ***
			unsigned int vtxIdxBBackup = triVtxIdxs[bestEdgeResult.vtxIDB];
			// Modify one vertex (the "B" one) in the current triangle. Getting a triangle with A - vtxToTreatIdx - vtxIDOther
			triVtxIdxs[bestEdgeResult.vtxIDB] = vtxToTreatIdx;
			// Force b-sphere rebuild
			_csgBspTrianglesBSphere[bestEdgeResult.triId].BuildFromTriangle(_csgBspVerticesResult[triVtxIdxs[bestEdgeResult.vtxIDA]], _csgBspVerticesResult[triVtxIdxs[bestEdgeResult.vtxIDB]], _csgBspVerticesResult[triVtxIdxs[bestEdgeResult.vtxIDOther]]);
			// Update triangle around vertex data
			CsgBspAddTriangleAroundVertex(triVtxIdxs[bestEdgeResult.vtxIDB], bestEdgeResult.triId);
			// *** Second triangle ***
			// Create a second triangle using vtxToTreatIdx - B - vtxIDOther
			// Take care: _triangles might be reallocated, all pointers on its data will be invalid
			unsigned int newTriIdx = CsgBspAddTriangle(vtxToTreatIdx, vtxIdxBBackup, triVtxIdxs[bestEdgeResult.vtxIDOther]);
			triVtxIdxs = &(_csgBspTrianglesResult[bestEdgeResult.triId * 3]);
			// Update triangle around vertex data
			CsgBspChangeTriangleAroundVertex(vtxIdxBBackup, bestEdgeResult.triId, newTriIdx);
			CsgBspAddTriangleAroundVertex(vtxToTreatIdx, newTriIdx);
			CsgBspAddTriangleAroundVertex(triVtxIdxs[bestEdgeResult.vtxIDOther], newTriIdx);
#ifdef MESH_CONSISTENCY_CHECK
			CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDA]);
			CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDB]);
			CheckVertexIsCorrect(triVtxIdxs[bestEdgeResult.vtxIDOther]);
			CheckVertexIsCorrect(vtxToTreatIdx);
#endif // MESH_CONSISTENCY_CHECK
		}
	}*/
	/*for(unsigned int unStitchedVtxId : unStitchedVtxIds)
	{
		if(CsgBspIsVertexToBeRemoved(unStitchedVtxId))
			continue;
		if(CheckVertexIsClosed(unStitchedVtxId))
			continue;
		if(_csgBspVtxToTriAround[unStitchedVtxId].size() <= 2)
			continue;
		bestEdgeResult.distance = DBL_MAX;
		bestEdgeResult.triId = UNDEFINED_NEW_ID;
		bestVertexResult.distance = DBL_MAX;
		bestVertexResult.vtxID = UNDEFINED_NEW_ID;
		// Scan for closest
		TestTriangles(unStitchedVtxId);
		// Vertex merge
		if(bestVertexResult.vtxID != UNDEFINED_NEW_ID)
		{
			MergeVertex(unStitchedVtxId, bestVertexResult.vtxID);
		}
	}*/
	// Clean vertices to be removed
	_csgBspTrianglesBSphere.clear();
	_csgBspVtxToTriAround.clear();
	unsigned int curVtxID = 0;
	unsigned int curNewVtxID = 0;
	std::map<unsigned int, unsigned int> oldToNewVtxID;
	for(bool removeVtx : _csgBspVtxsToBeRemoved)
	{
		if(!removeVtx)
		{
			oldToNewVtxID[curVtxID] = curNewVtxID;
			++curNewVtxID;
		}
		++curVtxID;		
	}
	for(auto& assoc : oldToNewVtxID)
	{
		if(assoc.first != assoc.second)
		{
			ASSERT(assoc.first > assoc.second);
			_csgBspVerticesResult[assoc.second] = _csgBspVerticesResult[assoc.first];
		}
	}
	_csgBspVerticesResult.resize(oldToNewVtxID.size());
	for(unsigned int& triVtxIdx : _csgBspTrianglesResult)
		triVtxIdx = oldToNewVtxID[triVtxIdx];
	_csgBspVtxsToBeRemoved.clear();
}
#endif // DO_STITCH_INTERSECTION_ZONE

#else

void Csg::PairLoops()
{
	auto LoopSquaredDistance = [&](Loop& loop1, Mesh& mesh1, Loop& loop2, Mesh& mesh2) -> float
	{
		float result = 0.0f;
		LoopElement const* curVtx1 = loop1.GetRoot();
		do
		{
			Vector3 const& vtx1 = mesh1.GetVertices()[curVtx1->GetVtxId()];
			float minSquaredDistance = FLT_MAX;
			LoopElement const* curVtx2 = loop2.GetRoot();
			do
			{
				Vector3 const& vtx2 = mesh2.GetVertices()[curVtx2->GetVtxId()];
				float squaredDistance = vtx1.DistanceSquared(vtx2);
				if(squaredDistance < minSquaredDistance)
					minSquaredDistance = squaredDistance;
				curVtx2 = curVtx2->GetNext();
			} while(curVtx2 != loop2.GetRoot());
			result += minSquaredDistance;
			curVtx1 = curVtx1->GetNext();
		} while(curVtx1 != loop1.GetRoot());
		return result;
	};
	ASSERT(_firstMeshLoops.size() == _secondMeshLoops.size());
	_firstToSecondMeshLoopPairs.clear();
	for(Loop& loopFirstMesh : _firstMeshLoops)
	{
		float minSquaredDistance = FLT_MAX;
		Loop* selectedLoop = &(_secondMeshLoops.front());
		for(Loop& loopSecondMesh : _secondMeshLoops)
		{
			float squaredDistance = LoopSquaredDistance(loopFirstMesh, _firstMesh, loopSecondMesh, _secondMesh);
			if(squaredDistance < minSquaredDistance)
			{
				minSquaredDistance = squaredDistance;
				selectedLoop = &loopSecondMesh;
			}
		}
		_firstToSecondMeshLoopPairs[&loopFirstMesh] = selectedLoop;
	}
}

void Csg::StitchProgress()
{
	printf("StitchProgress\n");
	printf("---1---\n");
	for(auto loopPair : _firstToSecondMeshLoopPairs)
	{
		LoopElement const* curVtx1 = loopPair.first->GetRoot();
		int count1 = 0;
		int count2 = 0;
		do
		{
			Vector3& vtx1 = _firstMesh.GrabVertices()[curVtx1->GetVtxId()];
			float minSquaredDistance = FLT_MAX;
			Vector3* closestVtx2 = nullptr;			
			LoopElement const* curVtx2 = loopPair.second->GetRoot();
			count2 = 0;
			do
			{
				Vector3& vtx2 = _secondMesh.GrabVertices()[curVtx2->GetVtxId()];
				float squaredDistance = vtx1.DistanceSquared(vtx2);
				if(squaredDistance < minSquaredDistance)
				{
					minSquaredDistance = squaredDistance;
					closestVtx2 = &vtx2;
				}
				count2++;
				curVtx2 = curVtx2->GetNext();
			} while(curVtx2 != loopPair.second->GetRoot());
			ASSERT(closestVtx2 != nullptr);
			if(closestVtx2 != nullptr)
			{
				Vector3 deltaDir = *closestVtx2 - vtx1;
				float deltaLength = deltaDir.Length();
				if(deltaLength != 0.0f)
				{
					deltaDir /= deltaLength;
					vtx1 += deltaDir * deltaLength * 0.2f;
				}
			}
			count1++;
			curVtx1 = curVtx1->GetNext();
		} while(curVtx1 != loopPair.first->GetRoot());
		printf("%d - %d\n", count1, count2);
	}
	printf("---2---\n");
	for(auto loopPair : _firstToSecondMeshLoopPairs)
	{
		LoopElement const* curVtx1 = loopPair.second->GetRoot();
		int count1 = 0;
		int count2 = 0;
		do
		{
			Vector3& vtx1 = _secondMesh.GrabVertices()[curVtx1->GetVtxId()];
			float minSquaredDistance = FLT_MAX;
			Vector3* closestVtx2 = nullptr;
			LoopElement const* curVtx2 = loopPair.first->GetRoot();
			count2 = 0;
			do
			{
				Vector3& vtx2 = _firstMesh.GrabVertices()[curVtx2->GetVtxId()];
				float squaredDistance = vtx1.DistanceSquared(vtx2);
				if(squaredDistance < minSquaredDistance)
				{
					minSquaredDistance = squaredDistance;
					closestVtx2 = &vtx2;
				}
				count2++;
				curVtx2 = curVtx2->GetNext();
			} while(curVtx2 != loopPair.first->GetRoot());
			ASSERT(closestVtx2 != nullptr);
			if(closestVtx2 != nullptr)
			{
				Vector3 deltaDir = *closestVtx2 - vtx1;
				float deltaLength = deltaDir.Length();
				if(deltaLength)
				{
					deltaDir /= deltaLength;
					vtx1 += deltaDir * deltaLength * 0.2f;
				}
			}
			count1++;
			curVtx1 = curVtx1->GetNext();
		} while(curVtx1 != loopPair.second->GetRoot());
		printf("%d - %d\n", count1, count2);
	}
}

void Csg::StitchFinalize()
{
	for(auto loopPair : _firstToSecondMeshLoopPairs)
	{
		LoopElement const* curVtx1 = loopPair.first->GetRoot();
		do
		{
			Vector3& vtx1 = _firstMesh.GrabVertices()[curVtx1->GetVtxId()];
			float minSquaredDistance = FLT_MAX;
			Vector3 const* closestVtx2 = nullptr;
			LoopElement const* curVtx2 = loopPair.second->GetRoot();
			do
			{
				Vector3 const& vtx2 = _secondMesh.GetVertices()[curVtx2->GetVtxId()];
				float squaredDistance = vtx1.DistanceSquared(vtx2);
				if(squaredDistance < minSquaredDistance)
				{
					minSquaredDistance = squaredDistance;
					closestVtx2 = &vtx2;
				}
				curVtx2 = curVtx2->GetNext();
			} while(curVtx2 != loopPair.second->GetRoot());
			ASSERT(closestVtx2 != nullptr);
			if(closestVtx2 != nullptr)
				vtx1 = *closestVtx2;
			curVtx1 = curVtx1->GetNext();
		} while(curVtx1 != loopPair.first->GetRoot());
	}
	for(auto loopPair : _firstToSecondMeshLoopPairs)
	{
		LoopElement const* curVtx1 = loopPair.second->GetRoot();
		do
		{
			Vector3& vtx1 = _secondMesh.GrabVertices()[curVtx1->GetVtxId()];
			float minSquaredDistance = FLT_MAX;
			Vector3 const* closestVtx2 = nullptr;
			LoopElement const* curVtx2 = loopPair.first->GetRoot();
			do
			{
				Vector3 const& vtx2 = _firstMesh.GetVertices()[curVtx2->GetVtxId()];
				float squaredDistance = vtx1.DistanceSquared(vtx2);
				if(squaredDistance < minSquaredDistance)
				{
					minSquaredDistance = squaredDistance;
					closestVtx2 = &vtx2;
				}
				curVtx2 = curVtx2->GetNext();
			} while(curVtx2 != loopPair.first->GetRoot());
			ASSERT(closestVtx2 != nullptr);
			if(closestVtx2 != nullptr)
				vtx1 = *closestVtx2;
			curVtx1 = curVtx1->GetNext();
		} while(curVtx1 != loopPair.second->GetRoot());
	}
}
#endif //!USE_BSP

void Csg::RebuildMeshFromCsg(bool recenter)
{
	struct BestEdgeResult
	{
		double distance;
		unsigned int triId;
		unsigned int vtxIDA;
		unsigned int vtxIDB;
		unsigned int vtxIDOther;
	} bestEdgeResult;
	struct BestVertexResult
	{
		double distance;
		unsigned int vtxID;
	} bestVertexResult;
	auto CheckVertexIsClosed = [&](unsigned int vtxIdx) -> bool	// Returns true if the vertex is already completely surrounded by the triangles it's linked to
	{
		for(unsigned int const& triIdx : _resultMesh.GetTrianglesAroundVertex(vtxIdx))
		{
			ASSERT(triIdx < _resultMesh.GetTrisNormal().size());
			unsigned int const* vtxsIdx = &(_resultMesh.GetTriangles()[triIdx * 3]);
			// *** edge shared by 2 triangles only check ****
			for(int i = 0; i < 3; ++i)
			{
				if(vtxsIdx[i] != vtxIdx)
				{	// Test that an edge is only shared by two triangles
					std::vector<unsigned int> const& triAroundA = _resultMesh.GetTrianglesAroundVertex(vtxIdx);
					std::vector<unsigned int> const& triAroundB = _resultMesh.GetTrianglesAroundVertex(vtxsIdx[i]);
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
	};
	auto GetVertexDist = [&](unsigned int vtxToTreatIdx, unsigned int vtxIdxToTestOnto) -> double
	{
		// Test that vtxToTreatIdx and vtxIdxToTestOnto are not connected
		for(unsigned int triAround : _resultMesh.GetTrianglesAroundVertex(vtxToTreatIdx))
		{
			unsigned int const* vtxIdxs = &(_resultMesh.GetTriangles()[triAround * 3]);
			for(unsigned int i = 0; i < 3; ++i)
			{
				if(vtxIdxs[i] == vtxIdxToTestOnto)
					return DBL_MAX;
			}
		}
		std::vector<Vector3> const& vertices = _resultMesh.GetVertices();
		Vector3Double A(double(vertices[vtxToTreatIdx].x), double(vertices[vtxToTreatIdx].y), double(vertices[vtxToTreatIdx].z));
		Vector3Double B(double(vertices[vtxIdxToTestOnto].x), double(vertices[vtxIdxToTestOnto].y), double(vertices[vtxIdxToTestOnto].z));
		return (A - B).Length();
	};
	auto MergeVertex = [&](unsigned int vtxToTreatIdx, unsigned int vtxIdxToTestOnto)
	{
		_resultMesh.SetVertexToBeRemoved(vtxToTreatIdx);
		for(unsigned int triIdxToRemapVtx : _resultMesh.GetTrianglesAroundVertex(vtxToTreatIdx))
		{
			unsigned int* triVtxIdx = &(_resultMesh.GrabTriangles()[triIdxToRemapVtx * 3]);
			for(unsigned int k = 0; k < 3; ++k)
			{
				if(triVtxIdx[k] == vtxToTreatIdx)
				{
					triVtxIdx[k] = vtxIdxToTestOnto;
					_resultMesh.AddTriangleAroundVertex(vtxIdxToTestOnto, triIdxToRemapVtx);
					break;
				}
			}
		}
		_resultMesh.ClearTriangleAroundVertex(vtxToTreatIdx);
	};
	auto TestRelyOnEdge = [&](unsigned int vtxToTreatIdx, unsigned int vtxIdxA, unsigned int vtxIdxB) -> double	// Return the distance from vtxToTreatIdx to A-B segment if they are pointing in the same direction and if A-vtxToTreatIdx is smaller than A-B (returns DBG_MAX instead)
	{
		std::vector<Vector3> const& vertices = _resultMesh.GetVertices();
		Vector3Double A(double(vertices[vtxIdxA].x), double(vertices[vtxIdxA].y), double(vertices[vtxIdxA].z));
		Vector3Double B(double(vertices[vtxIdxB].x), double(vertices[vtxIdxB].y), double(vertices[vtxIdxB].z));
		Vector3Double Other(double(vertices[vtxToTreatIdx].x), double(vertices[vtxToTreatIdx].y), double(vertices[vtxToTreatIdx].z));
		Vector3Double AToB = B - A;
		Vector3Double AToOther = Other - A;
		double lengthAToB = AToB.Length();
		double lengthAToOther = AToOther.Length();
		//ASSERT(lengthAToOther != 0.0);
		if(lengthAToOther > lengthAToB)
			return DBL_MAX;
		if(lengthAToB == 0.0)
			return DBL_MAX;
		Vector3Double AToBDir = AToB / lengthAToB;
		double dot = AToBDir.Dot(AToOther);
		if(dot < 0.0)
			return DBL_MAX;
		Vector3Double projPointOnSegment = (AToBDir * dot) + A;
		return (Other - projPointOnSegment).Length();
	};
	auto TestTriangles = [&](std::vector<unsigned int> const& triangles, unsigned int vtxToTreatIdx)	// Will update bestVertexResult and bestEdgeResult
	{
		for(unsigned int i = 0; (bestVertexResult.distance != 0.0f) && (bestEdgeResult.distance != 0.0f) && (i < triangles.size()); ++i)
		{
			unsigned int triIdx = triangles[i];
			BSphere const& triBSphere = _resultMesh.GetTrisBSphere()[triIdx];
			if(triBSphere.Intersects(_resultMesh.GetVertices()[vtxToTreatIdx], EPSILON))		// Check for each vertex if it touches a triangle b-sphere
			{	// Intersects
				// Test if that triangle doesn't use that vertex already
				unsigned int* triVtxIdxs = &(_resultMesh.GrabTriangles()[triIdx * 3]);
				if((triVtxIdxs[0] != vtxToTreatIdx) && (triVtxIdxs[1] != vtxToTreatIdx) && (triVtxIdxs[2] != vtxToTreatIdx))
				{	// If that's not the case, then test if the vertex rely on one of the triangle edge
					double distVertex = GetVertexDist(vtxToTreatIdx, triVtxIdxs[0]);
					if(distVertex < bestVertexResult.distance)
					{
						bestVertexResult.distance = distVertex;
						bestVertexResult.vtxID = triVtxIdxs[0];
					}
					distVertex = GetVertexDist(vtxToTreatIdx, triVtxIdxs[1]);
					if(distVertex < bestVertexResult.distance)
					{
						bestVertexResult.distance = distVertex;
						bestVertexResult.vtxID = triVtxIdxs[1];
					}
					distVertex = GetVertexDist(vtxToTreatIdx, triVtxIdxs[2]);
					if(distVertex < bestVertexResult.distance)
					{
						bestVertexResult.distance = distVertex;
						bestVertexResult.vtxID = triVtxIdxs[2];
					}
					double dists[3];
					dists[0] = TestRelyOnEdge(vtxToTreatIdx, triVtxIdxs[0], triVtxIdxs[1]);
					dists[1] = TestRelyOnEdge(vtxToTreatIdx, triVtxIdxs[1], triVtxIdxs[2]);
					dists[2] = TestRelyOnEdge(vtxToTreatIdx, triVtxIdxs[2], triVtxIdxs[0]);
					double minDist = min(min(dists[0], dists[1]), dists[2]);	// On very flat triangles, cosine could approx 1.0 on more than one edge
					if(minDist < bestEdgeResult.distance)
					{
						bestEdgeResult.distance = minDist;
						bestEdgeResult.triId = triIdx;
						if((dists[0] <= dists[1]) && (dists[0] <= dists[2]))
						{
							bestEdgeResult.vtxIDA = 0;
							bestEdgeResult.vtxIDB = 1;
							bestEdgeResult.vtxIDOther = 2;
						}
						else if((dists[1] <= dists[0]) && (dists[1] <= dists[2]))
						{
							bestEdgeResult.vtxIDA = 1;
							bestEdgeResult.vtxIDB = 2;
							bestEdgeResult.vtxIDOther = 0;
						}
						else // if((dists[2] <= dists[1]) && (dists[2] <= dists[0]))
						{
							bestEdgeResult.vtxIDA = 2;
							bestEdgeResult.vtxIDB = 0;
							bestEdgeResult.vtxIDOther = 1;
						}
					}
				}
			}
		}
	};

	// Add the tess on first and second mesh that was not in the intersection zone
	ExtractRemainingMeshTess(_firstMesh, _resultVertices, _resultTriangles);
	ExtractRemainingMeshTess(_secondMesh, _resultVertices, _resultTriangles);

	// Build a mesh
	_resultMesh.GrabVertices() = std::move(_resultVertices);
	_resultMesh.GrabTriangles() = std::move(_resultTriangles);
	_resultMesh.RebuildMeshData(false, recenter, false);

#ifdef DO_STITCH_WHOLE_MESH
	// *** Will subdivide triangle to link with vertices that lies on the edge of another triangle ***
	// Merge CSG generated vertices
	// For each CSG created vertex we will try to find a triangle with an edge clearly touching
	ASSERT(_resultMesh._trisIdxToRecycle.size() == 0);	// We assume we don't have triangle to recycle in the code below
	std::vector<unsigned int> createdTriangles;
	//std::vector<unsigned int> unStitchedVtxIds;
	std::vector<Vector3> const& vertices = _resultMesh.GetVertices();
	for(unsigned int vtxToTreatIdx = 0; vtxToTreatIdx < _csgBspVerticesResult.size(); ++vtxToTreatIdx)
	{
		bestEdgeResult.distance = DBL_MAX;
		bestEdgeResult.triId = UNDEFINED_NEW_ID;
		bestVertexResult.distance = DBL_MAX;
		bestVertexResult.vtxID = UNDEFINED_NEW_ID;
		// Scan for closest
		VisitorGetOctreeVertexIntersection _octreeVertexIntersection(vertices[vtxToTreatIdx]);
		_resultMesh.GrabOctreeRoot().Traverse(_octreeVertexIntersection);
		for(OctreeCell const* cell : _octreeVertexIntersection.GetCollidedCells())
			TestTriangles(cell->GetTrianglesIdx(), vtxToTreatIdx);
		TestTriangles(createdTriangles, vtxToTreatIdx);
		// Vertex merge
		if(bestVertexResult.distance < Csg_Bsp::Plane::_EPSILON)
		{
			MergeVertex(vtxToTreatIdx, bestVertexResult.vtxID);
		}
		// Edge split
		else if(bestEdgeResult.triId != UNDEFINED_NEW_ID)
		{
			//ASSERT(bestResult.distance < EPSILON_DBL);
			if(CheckVertexIsClosed(vtxToTreatIdx))
				continue;
			if(bestEdgeResult.distance >= EPSILON_DBL)
			{
				//unStitchedVtxIds.push_back(vtxToTreatIdx);
				continue;		// Could happen if the vertex is already completely surrounded by the triangles it's linked to
			}
			unsigned int* triVtxIdxs = &(_resultMesh.GrabTriangles()[bestEdgeResult.triId * 3]);
			// *** First triangle ***
			unsigned int vtxIdxBBackup = triVtxIdxs[bestEdgeResult.vtxIDB];
			// Modify one vertex (the "B" one) in the current triangle. Getting a triangle with A - vtxToTreatIdx - vtxIDOther
			triVtxIdxs[bestEdgeResult.vtxIDB] = vtxToTreatIdx;
			// Force b-sphere rebuild
			_resultMesh.GrabTrisBSphere()[bestEdgeResult.triId].BuildFromTriangle(vertices[triVtxIdxs[bestEdgeResult.vtxIDA]], vertices[triVtxIdxs[bestEdgeResult.vtxIDB]], vertices[triVtxIdxs[bestEdgeResult.vtxIDOther]]);
			// Update triangle around vertex data
			_resultMesh.AddTriangleAroundVertex(triVtxIdxs[bestEdgeResult.vtxIDB], bestEdgeResult.triId);
			// *** Second triangle ***
			// Create a second triangle using vtxToTreatIdx - B - vtxIDOther
			// Take care: _triangles might be reallocated, all pointers on its data will be invalid
			unsigned int newTriIdx = _resultMesh.AddTriangle(vtxToTreatIdx, vtxIdxBBackup, triVtxIdxs[bestEdgeResult.vtxIDOther], _resultMesh.GetTrisNormal()[bestEdgeResult.triId], true);
			triVtxIdxs = &(_resultMesh.GrabTriangles()[bestEdgeResult.triId * 3]);
			createdTriangles.push_back(newTriIdx);
			// Update triangle around vertex data
			_resultMesh.ChangeTriangleAroundVertex(vtxIdxBBackup, bestEdgeResult.triId, newTriIdx);
			_resultMesh.AddTriangleAroundVertex(vtxToTreatIdx, newTriIdx);
			_resultMesh.AddTriangleAroundVertex(triVtxIdxs[bestEdgeResult.vtxIDOther], newTriIdx);
		}
	}
	_resultMesh.HandlePendingRemovals();	
	_resultMesh.RecomputeNormals(false);
	_resultMesh.TagAndCollectOpenEdgesVertices(nullptr);
	std::vector<unsigned int> createdVertices;
	_resultMesh.ProcessHardEdges(&createdTriangles, &createdVertices);
	
	_resultMesh.ReBalanceOctree(createdTriangles, createdVertices, true);
#endif // DO_STITCH_WHOLE_MESH

#if 1
	_resultMesh.RecomputeNormals(false);	// Todo: perhaps could prevent two calls to "RecomputeNormals", may have some redundancy
	_resultMesh.RecomputeFragmentsBBox(false);
#else
	_resultMesh.RebuildMeshData(false, false, true);
#endif
	
#ifdef MESH_CONSISTENCY_CHECK
	_resultMesh.CheckMeshIsCorrect();
#endif // MESH_CONSISTENCY_CHECK
	printf("CSG rebuild done: triangle count %d, vertex count %d\n", (int) _resultMesh.GetTriangles().size() / 3, (int) _resultMesh.GetVertices().size());

	// Take snapshot for undo/redo
	_resultMesh.TakeSnapShot();
}
