#include "OctreeVisitorBuildAndCollectSubMeshes.h"
#include "Octree.h"
#include "Mesh.h"
#include "SubMesh.h"

VisitorBuildAndCollectSubMeshes::VisitorBuildAndCollectSubMeshes(Mesh& fullMesh, VisitorBuildAndCollectSubMeshes const& cloneFrom): OctreeVisitor(), _fullMesh(fullMesh)
{
	for(auto subMeshEntry : cloneFrom._subMeshes)
		_subMeshes[subMeshEntry.first].reset(new SubMesh(*(subMeshEntry.second)));
	_subMeshesIds = cloneFrom._subMeshesIds;
	_subMeshesToDelete = cloneFrom._subMeshesToDelete;
}

bool VisitorBuildAndCollectSubMeshes::HasToVisit(OctreeCell& /*cell*/)
{
	return true;
}

void VisitorBuildAndCollectSubMeshes::VisitEnter(OctreeCell& cell)
{
	if(cell.IsRoot())
	{	// Start traverse
		_subMeshesToDelete.clear();
		for(auto subMeshEntry : _subMeshes)
			_subMeshesToDelete.insert(subMeshEntry.first);
	}
	if(!cell.GetTrianglesIdx().empty())
	{
		_subMeshesToDelete.erase(cell.GetID());	// Sub mesh (so octree cell) still exist
		if(cell.TestStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH) == false)
		{	// Do a deep test to ensure that some triangles hasn't been modified without the cell flag set: When doing dynamic tessellation (subdiv and merge), it could spread to surroundings triangles and vertices. And keeping track of the triangle and vertices to octree cell association could cost much in memory and link update (from vertex or triangle to octree cell), so we set flag on vertices and scan for it
			std::vector<unsigned int> const& trianglesIdx = cell.GetTrianglesIdx();
			std::vector<unsigned int> const& trianglesFullmesh = _fullMesh.GetTriangles();
			for(unsigned int const& triIdx : trianglesIdx)
			{
				unsigned int const* vtxsIdx = &(trianglesFullmesh[triIdx * 3]);
				if(_fullMesh.IsVertexToUpdateInSubMesh(vtxsIdx[0])
				|| _fullMesh.IsVertexToUpdateInSubMesh(vtxsIdx[1])
				|| _fullMesh.IsVertexToUpdateInSubMesh(vtxsIdx[2]))
				{
					cell.AddStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH);
					break;
				}
			}
		}
		if(cell.TestStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH))
		{
			cell.ClearStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH);
			// Try to find an already existing SubMesh, and if not, create it
			std::map<unsigned int, std::shared_ptr<SubMesh>>::iterator itFindSubMesh = _subMeshes.find(cell.GetID());
			std::shared_ptr<SubMesh> subMesh;
			if(itFindSubMesh == _subMeshes.end())
			{
				subMesh.reset(new SubMesh(cell.GetID()));
				_subMeshes[cell.GetID()] = subMesh;
			}
			else
			{
				subMesh = itFindSubMesh->second;
				subMesh->IncVersionNumber();	// This way the caller/user of the submesh will know he has to update its data
			}
			// Get required vectors
			std::vector<unsigned int> const& trianglesFullmesh = _fullMesh.GetTriangles();
			std::vector<Vector3> const& verticesFullmesh = _fullMesh.GetVertices();
			std::vector<Vector3> const& normalsFullmesh = _fullMesh.GetNormals();
			std::vector<unsigned int> const& trianglesIdx = cell.GetTrianglesIdx();
			std::vector<unsigned int>& subMeshTriangles = subMesh->GrabTriangles();
			std::vector<Vector3>& subMeshVertioes = subMesh->GrabVertices();
			std::vector<Vector3>& subMeshNormals = subMesh->GrabNormals();
			// todo : enhance performances of the code below
			subMeshTriangles.clear();
			subMeshVertioes.clear();
			subMeshNormals.clear();
			// Build the list of needed unique vertex indices
			std::vector<int> reqVtxIndices;
			reqVtxIndices.reserve(trianglesIdx.size() * 3);	// Of course we reserve too much memory (this would be the size we would get if no triangles were weld together), but that's worth it as we don't know the exact size we will need and to avoid to have to reallocate after
			for(unsigned int const& triIdx : trianglesIdx)
			{
				unsigned int const* vtxsIdx = &(trianglesFullmesh[triIdx * 3]);
				for(int i = 0; i < 3; ++i)
				{
					if(!_fullMesh.IsVertexAlreadyTreated(vtxsIdx[i]))
					{
						_fullMesh.SetVertexAlreadyTreated(vtxsIdx[i]);
						reqVtxIndices.push_back(vtxsIdx[i]);
					}
				}
			}
			// Build vertex vector from collected unique vertex indices
			subMeshVertioes.reserve(reqVtxIndices.size());
			subMeshNormals.reserve(reqVtxIndices.size());
			std::map<int, int> vtxIdxRemap;
			for(int const& vtxIdx : reqVtxIndices)
			{
				vtxIdxRemap[vtxIdx] = (int) subMeshVertioes.size();
				subMeshVertioes.push_back(verticesFullmesh[vtxIdx]);
				subMeshNormals.push_back(normalsFullmesh[vtxIdx]);
				_fullMesh.ClearVertexAlreadyTreated(vtxIdx);
			}
			// Build triangles draw list
			subMeshTriangles.reserve(trianglesIdx.size() * 3);
			for(unsigned int const& triIdx : trianglesIdx)
			{
				unsigned int const* vtxsIdx = &(trianglesFullmesh[triIdx * 3]);
				for(int i = 0; i < 3; ++i)
					subMeshTriangles.push_back(vtxIdxRemap[vtxsIdx[i]]);
			}
			// Set bbox
			subMesh->SetBBox(cell.GetContentBBox());
		}
#ifdef _DEBUG
		std::map<unsigned int, std::shared_ptr<SubMesh>>::iterator itFindSubMesh = _subMeshes.find(cell.GetID());
		if(itFindSubMesh != _subMeshes.end())
		{
			//ASSERT(itFindSubMesh->second->GetVertices().size() == cell.GetVerticesIdx().size());	// Can't test vertices number as vertices stored in the cell doesn't necessarily belong to the cell's triangle.
			ASSERT(itFindSubMesh->second->GetTriangles().size() == cell.GetTrianglesIdx().size() * 3);
		}
#endif // _DEBUG
	}
}

void VisitorBuildAndCollectSubMeshes::VisitLeave(OctreeCell& cell)
{
	if(cell.IsRoot())
	{	// End traverse
		// Remove all submeshes detected as deleted
		for(unsigned int idSubMeshToDelete : _subMeshesToDelete)
			_subMeshes.erase(idSubMeshToDelete);
		// Rebuild our object ID array
		_subMeshesIds.clear();
		for(auto subMeshEntry : _subMeshes)
			_subMeshesIds.push_back(subMeshEntry.first);
		// Clear VTX_STATE_HAS_TO_RECOMPUTE_SUB_MESH on all fullmesh vertices
		_fullMesh.ClearAllVerticesToUpdateInSubMesh();
	}
}