#include "OctreeVisitorRetessellateInRange.h"
#include "Octree.h"
#include "Mesh.h"

//#define MULTI_PASS_RETESS

void VisitorRetessellateInRange::VisitEnter(OctreeCell& cell)
{
	if(!cell.GetTrianglesIdx().empty() || !cell.GetVerticesIdx().empty())
	{
		Retessellate& retessellate = _mesh.GrabRetessellator();
		unsigned int nbNewTriBeforeBegin = (unsigned int) retessellate.GetGenratedTris().size();
		// Remove flat triangles: their smallest height get close to zero
		bool somethingWasModified = false;
#ifdef MULTI_PASS_RETESS
		unsigned int loopCount = 0;
		do
		{
			somethingWasModified = false;
#endif // MULTI_PASS_RETESS
			bool somethingWasSubdivided = retessellate.WasSomethingSubdivided();
			{
				retessellate.SetSomethingSubdivided(false);
#ifdef MULTI_PASS_RETESS
				// handle newly created triangle in the cell
				unsigned int nbGenTrisBeforeLoop = (unsigned int) retessellate.GetGenratedTris().size();
				for(unsigned int i = nbNewTriBeforeBegin; i < nbGenTrisBeforeLoop; ++i)
					HandleCrushedTriangle(retessellate.GetGenratedTris()[i]);
#endif // MULTI_PASS_RETESS
				// Handle cell triangles
				std::vector<unsigned int> const& cellTrianglesIdx = cell.GetTrianglesIdx();
				for(unsigned int triIdx : cellTrianglesIdx)
					HandleCrushedTriangle(triIdx);
				somethingWasModified = somethingWasModified || retessellate.WasSomethingSubdivided();
				somethingWasSubdivided = somethingWasSubdivided || retessellate.WasSomethingSubdivided();
			}
			retessellate.SetSomethingSubdivided(somethingWasSubdivided);

			// Merge triangles with edges smaller than "d"
			bool somethingWasMerged = retessellate.WasSomethingMerged();
			do
			{
				retessellate.SetSomethingMerged(false);
				// handle newly created triangle in the cell
				unsigned int nbGenTrisBeforeLoop = (unsigned int) retessellate.GetGenratedTris().size();
				for(unsigned int i = nbNewTriBeforeBegin; i < nbGenTrisBeforeLoop; ++i)
					HandleTriangleMerge(retessellate.GetGenratedTris()[i]);
				// Handle cell triangles
				std::vector<unsigned int> const& cellTrianglesIdx = cell.GetTrianglesIdx();
				for(unsigned int triIdx : cellTrianglesIdx)
					HandleTriangleMerge(triIdx);
				somethingWasModified = somethingWasModified || retessellate.WasSomethingMerged();
				somethingWasMerged = somethingWasMerged || retessellate.WasSomethingMerged();
			} while(retessellate.WasSomethingMerged());
			retessellate.SetSomethingMerged(somethingWasMerged);	// For the caller to be able if something was merged during the process

			// Subdivide triangles with edges taller than "d detail"
			somethingWasSubdivided = retessellate.WasSomethingSubdivided();
			do
			{
				retessellate.SetSomethingSubdivided(false);
				// handle newly created triangle in the cell
				unsigned int nbGenTrisBeforeLoop = (unsigned int) retessellate.GetGenratedTris().size();
				for(unsigned int i = nbNewTriBeforeBegin; i < nbGenTrisBeforeLoop; ++i)
					HandleTriangleSubdiv(retessellate.GetGenratedTris()[i], true, true);
				// Handle cell triangles
				std::vector<unsigned int> const& cellTrianglesIdx = cell.GetTrianglesIdx();
				for(unsigned int triIdx : cellTrianglesIdx)
					HandleTriangleSubdiv(triIdx, true, true);
				somethingWasModified = somethingWasModified || retessellate.WasSomethingSubdivided();
				somethingWasSubdivided = somethingWasSubdivided || retessellate.WasSomethingSubdivided();
			} while(retessellate.WasSomethingSubdivided());
			retessellate.SetSomethingSubdivided(somethingWasSubdivided);	// For the caller to be able if something was subdivided during the process
#ifdef MULTI_PASS_RETESS
		} while(somethingWasModified && (++loopCount < 3));
#endif // MULTI_PASS_RETESS
		// If we modified something, setup update flags accordingly
		if(retessellate.WasSomethingSubdivided() || retessellate.WasSomethingMerged())
		{
			cell.AddStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH | CELL_STATE_HASTO_EXTRACT_OUTOFBOUNDS_GEOM);
			cell.AddStateFlagsUpToRoot(CELL_STATE_HASTO_RECOMPUTE_BBOX);
		}
	}
}

bool VisitorRetessellateInRange::HandleTriangleSubdiv(unsigned int triIdx, bool testDistance, bool testLength)
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
	//if(!testDistance || Intersects(_trisBSphere[triIdx]))
	if(!testDistance || Intersects(BSphere(vtx0, vtx1, vtx2)))
	{
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
	return false;
}

void VisitorRetessellateInRange::HandleTriangleMerge(unsigned int triIdx)
{
	if(_mesh.IsTriangleToBeRemoved(triIdx))
		return;	// Don't treat triangles that are to be removed
	std::vector<unsigned int> const& triangles = _mesh.GetTriangles();
	std::vector<Vector3>& vertices = _mesh.GrabVertices();
	unsigned int const* vtxsIdx = &(triangles[triIdx * 3]);
	unsigned int vtxId0 = vtxsIdx[0];
	unsigned int vtxId1 = vtxsIdx[1];
	unsigned int vtxId2 = vtxsIdx[2];
	Vector3 const& vtx0 = vertices[vtxId0];
	Vector3 const& vtx1 = vertices[vtxId1];
	Vector3 const& vtx2 = vertices[vtxId2];
	//if(Intersects(_trisBSphere[triIdx]))
	if(Intersects(BSphere(vtx0, vtx1, vtx2)))
	{
		// will ask to merge the smallest edge
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
#ifdef MESH_CONSISTENCY_CHECK
		_mesh.CheckVertexIsCorrect(vtxId0);
		_mesh.CheckVertexIsCorrect(vtxId1);
		_mesh.CheckVertexIsCorrect(vtxId2);
#endif // MESH_CONSISTENCY_CHECK
	}
}

void VisitorRetessellateInRange::HandleCrushedTriangle(unsigned int triIdx)
{
	if(_mesh.IsTriangleToBeRemoved(triIdx))
		return;	// Don't treat triangles that are to be removed
#ifdef MULTI_PASS_RETESS
	std::vector<unsigned int> const& triangles = _mesh.GetTriangles();
	std::vector<Vector3>& vertices = _mesh.GrabVertices();
	unsigned int const* vtxsIdx = &(triangles[triIdx * 3]);
	unsigned int vtxId0 = vtxsIdx[0];
	unsigned int vtxId1 = vtxsIdx[1];
	unsigned int vtxId2 = vtxsIdx[2];
	Vector3 const& vtx0 = vertices[vtxId0];
	Vector3 const& vtx1 = vertices[vtxId1];
	Vector3 const& vtx2 = vertices[vtxId2];
	if(Intersects(BSphere(vtx0, vtx1, vtx2)))
#else
	if(Intersects(_mesh.GetTrisBSphere()[triIdx]))
#endif // !MULTI_PASS_RETESS
		_mesh.GrabRetessellator().RemoveCrushedTriangle(triIdx, false);
}
