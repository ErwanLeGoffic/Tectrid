#include "Octree.h"
#include "Mesh.h"

const unsigned int maxTrianglesPerCell = 1000;
unsigned int OctreeCell::_idGen = 0;

OctreeCell::OctreeCell(OctreeCell const& otherCell, OctreeCell* parent):
	_parent(parent),
	_trianglesIdx(otherCell._trianglesIdx),
	_verticesIdx(otherCell._verticesIdx),
	_BBox(otherCell._BBox),
	_contentBBox(otherCell._contentBBox),
	_id(otherCell._id),
	_stateFlags(otherCell._stateFlags)
{
	_children.resize(otherCell._children.size());
	unsigned int i = 0;
	for(std::unique_ptr<OctreeCell> const& otherCellChild : otherCell._children)
	{
		if(otherCellChild != nullptr)
			_children[i++].reset(new OctreeCell(*otherCellChild, this));
	}
}

void OctreeCell::Insert(Mesh const& mesh, std::vector<unsigned int> const& trisToInsert, std::vector<unsigned int> const& vtxsToInsert)
{
	std::vector<Vector3> const& vertices = mesh.GetVertices();
	std::vector<unsigned int> const& triangles = mesh.GetTriangles();
	unsigned int totalCellTris = (unsigned int) (trisToInsert.size() + _trianglesIdx.size());
	if(HasChildren() || (totalCellTris > maxTrianglesPerCell))	// have to subdivide cells or keep subdivision if it's already done
	{
		// Build sub cell bbox
		Vector3 const& center = _BBox.Center();
		Vector3 const& min = _BBox.Min();
		Vector3 const& max = _BBox.Max();
		std::vector<BBox> subCellsBBox;
		subCellsBBox.reserve(8);
		subCellsBBox.push_back(BBox(min, center));
		subCellsBBox.push_back(BBox(Vector3(max.x, min.y, min.z), center));
		subCellsBBox.push_back(BBox(Vector3(max.x, min.y, max.z), center));
		subCellsBBox.push_back(BBox(Vector3(min.x, min.y, max.z), center));
		subCellsBBox.push_back(BBox(max, center));
		subCellsBBox.push_back(BBox(Vector3(min.x, max.y, max.z), center));
		subCellsBBox.push_back(BBox(Vector3(min.x, max.y, min.z), center));
		subCellsBBox.push_back(BBox(Vector3(max.x, max.y, min.z), center));
		// Get all the triangles that fit in the sub-cells
		std::vector<unsigned int> trisInSubCells[8];
		for(std::vector<unsigned int>& trisInSubCell : trisInSubCells)
			trisInSubCell.reserve(totalCellTris);
		auto SetupSubCellTris = [&](std::vector<unsigned int> const& triIdxs)
		{
			for(int const& i : triIdxs)
			{
				int zoneIndex = 0;
				for(BBox const& subCellBBox : subCellsBBox)
				{
					if(subCellBBox.Contains(vertices[triangles[i * 3]]))	// just test first vertex for the moment (will center triangle center later)
					{
						trisInSubCells[zoneIndex].push_back(i);
						break;	// If a triangle fits in a sub cell, it won't fit in any other
					}
					++zoneIndex;
				}
				ASSERT(zoneIndex < 8);	// The triangle has no bbox it fits in
			}
		};
		SetupSubCellTris(trisToInsert);
		SetupSubCellTris(_trianglesIdx);
#ifdef _DEBUG
		// Verify that all triangles were set to the subcells
		unsigned int nbTotalTriInSubCells = 0;
		for(std::vector<unsigned int>& trisInSubCell : trisInSubCells)
			nbTotalTriInSubCells += (unsigned int) trisInSubCell.size();
		ASSERT(nbTotalTriInSubCells == (unsigned int) (trisToInsert.size() + _trianglesIdx.size()));
#endif // _DEBUG
		_trianglesIdx.clear();	// No need to set CELL_STATE_HASTO_UPDATE_SUB_MESH (if a cell is empty, the submesh will be deleted (see VisitorBuildAndCollectSubMeshes))
		_trianglesIdx.shrink_to_fit();
		for(std::vector<unsigned int>& trisInSubCell : trisInSubCells)
			trisInSubCell.shrink_to_fit();
		// Get all the vertices that fit in the sub-cells
		std::vector<unsigned int> vtxsInSubCells[8];
		for(std::vector<unsigned int>& vtxInSubCell : vtxsInSubCells)
			vtxInSubCell.reserve(vtxsToInsert.size());
		auto SetupSubCellVtxs = [&](std::vector<unsigned int> const& vtxIdxs)
		{
			for(int const& i : vtxIdxs)
			{
				int zoneIndex = 0;
				for(BBox const& subCellBBox : subCellsBBox)
				{
					if(subCellBBox.Contains(vertices[i]))
					{
						vtxsInSubCells[zoneIndex].push_back(i);
						break;	// If a vertex fits in a sub cell, it won't fit in any other
					}
					++zoneIndex;
				}
				ASSERT(zoneIndex < 8);	// The vertex has no bbox it fits in
			}
		};
		SetupSubCellVtxs(vtxsToInsert);
		SetupSubCellVtxs(_verticesIdx);
#ifdef _DEBUG
		// Verify that all vertices were set to the subcells
		unsigned int nbTotalVtxsInSubCells = 0;
		for(std::vector<unsigned int>& vtxsInSubCell : vtxsInSubCells)
			nbTotalVtxsInSubCells += (unsigned int) vtxsInSubCell.size();
		ASSERT(nbTotalVtxsInSubCells == (unsigned int) (vtxsToInsert.size() + _verticesIdx.size()));
#endif // _DEBUG
		_verticesIdx.clear();	// No need to set CELL_STATE_HASTO_UPDATE_SUB_MESH (if a cell is empty, the submesh will be deleted (see VisitorBuildAndCollectSubMeshes))
		_verticesIdx.shrink_to_fit();
		for(std::vector<unsigned int>& vtxInSubCell : vtxsInSubCells)
			vtxInSubCell.shrink_to_fit();
		// Create the sub cells and move the triangles and vertices that fit in each one
		_children.resize(8);
		int zoneIndex = 0;
		for(unsigned int i = 0; i < 8; ++i)
		{
			std::vector<unsigned int> const& trisInSubCell = trisInSubCells[i];
			std::vector<unsigned int> const& vtxsInSubCell = vtxsInSubCells[i];
			std::unique_ptr<OctreeCell>& _child = _children[zoneIndex];
			if(!trisInSubCell.empty() || !vtxsInSubCell.empty())
			{
				if(_child == nullptr)
					_child.reset(new OctreeCell(subCellsBBox[zoneIndex], this));
				_child->Insert(mesh, trisInSubCell, vtxsInSubCell);
			}
			++zoneIndex;
		}
	}
	else // Enough room for all triangles, don't need to subdivide, then consume the triangles and vertices and store them
	{
		if(_trianglesIdx.empty())
			_trianglesIdx = trisToInsert;	// Don't use std::move as the memory reserved in "trisToInsert" (and "vtxsToInsert") oversize the actual size
		else
		{	// Append trisToInsert to _trianglesIdx
			_trianglesIdx.reserve(_trianglesIdx.size() + trisToInsert.size());
			_trianglesIdx.insert(_trianglesIdx.end(), trisToInsert.begin(), trisToInsert.end());
		}
		if(_verticesIdx.empty())
			_verticesIdx = vtxsToInsert;	// Don't use std::move as the memory reserved in "trisToInsert" (and "vtxsToInsert") oversize the actual size
		else
		{	// Append vtxsToInsert to _verticesIdx
			_verticesIdx.reserve(_verticesIdx.size() + vtxsToInsert.size());
			_verticesIdx.insert(_verticesIdx.end(), vtxsToInsert.begin(), vtxsToInsert.end());
		}
		// have to recompute bbox and rebuild submesh
		AddStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH);
		AddStateFlagsUpToRoot(CELL_STATE_HASTO_RECOMPUTE_BBOX);
	}
}

void OctreeCell::Traverse(OctreeVisitor& visitor)
{
	if(visitor.HasToVisit(*this))
	{
		visitor.VisitEnter(*this);
		for(std::unique_ptr<OctreeCell>& childCell : _children)
		{
			if(childCell != nullptr)
				childCell->Traverse(visitor);
		}
		visitor.VisitLeave(*this);
	}
}

bool OctreeCell::HasChildren()
{
	if(_children.empty())
		return false;
	for(std::unique_ptr<OctreeCell> const& child : _children)
	{
		if(child != nullptr)
			return true;
	}
	return false;
}

void OctreeCell::ExtractOutOfBoundsGeom(Mesh const& mesh, std::vector<unsigned int>& extractedTris, std::vector<unsigned int>& extractedVtxs)
{
	std::vector<Vector3> const& vertices = mesh.GetVertices();
	std::vector<unsigned int> const& triangles = mesh.GetTriangles();
	size_t initialVerticesIdxSize = _verticesIdx.size();
	size_t initialTrianglesIdxSize = _trianglesIdx.size();

	for(unsigned int i = 0; i < _trianglesIdx.size();)
	{
		unsigned int triIdx = _trianglesIdx[i];
		if(!_BBox.Contains(vertices[triangles[triIdx * 3]]))	// just test first vertex for the moment (will center triangle center later)
		{	// Has to remove triangle from the cell
			// Remove element from triangle array
			_trianglesIdx[i] = _trianglesIdx.back();
			_trianglesIdx.pop_back();
			// Store the triangle ID we will have to insert again
			extractedTris.push_back(triIdx);
		}
		else
			++i;
	}
	for(unsigned int i = 0; i < _verticesIdx.size();)
	{
		unsigned int vtxIdx = _verticesIdx[i];
		if(!_BBox.Contains(vertices[vtxIdx]))
		{	// Has to remove vertex from the cell
			// Remove element from vertex array
			_verticesIdx[i] = _verticesIdx.back();
			_verticesIdx.pop_back();
			// Store the triangle ID we will have to insert again
			extractedVtxs.push_back(vtxIdx);
		}
		else
			++i;
	}
	// If something has changed, we have to recompute bbox and rebuild submesh
	if((initialVerticesIdxSize != _verticesIdx.size()) || (initialTrianglesIdxSize != _trianglesIdx.size()))
	{
		AddStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH);
		AddStateFlagsUpToRoot(CELL_STATE_HASTO_RECOMPUTE_BBOX);
	}
}

void OctreeCell::PurgeEmptyChildren()
{
	if(_children.empty())
		return;
	bool hasAValidChild = false;
	for(std::unique_ptr<OctreeCell>& childCell : _children)
	{
		if(childCell != nullptr)
		{
			if(!childCell->HasChildren() && childCell->GetTrianglesIdx().empty() && childCell->GetVerticesIdx().empty())
				childCell = nullptr;
			else
				hasAValidChild = true;
		}
	}
	if(hasAValidChild == false)	// No child with something in it are there any more. So delete children array
	{
		_children.clear();
		_children.shrink_to_fit();
	}
}

void OctreeCell::HandlePendingRemovals(Mesh const& mesh, bool doRemapping)
{
	// remove from vertices and triangles, the ones that have been tagged has pending to be removed (because one of their edge were too small)
	size_t initialVerticesIdxSize = _verticesIdx.size();
	size_t initialTrianglesIdxSize = _trianglesIdx.size();
	bool remapOccured = false;
	for(unsigned int i = 0; i < _trianglesIdx.size();)
	{
		unsigned int& triIdx = _trianglesIdx[i];
		if(mesh.IsTriangleToBeRemoved(triIdx))
		{	// Remove element
			triIdx = _trianglesIdx.back();
			_trianglesIdx.pop_back();
		}
		else
		{
			if(doRemapping && mesh.TriHasToMove(triIdx))	// Do remapping if not asked (save memory load access costs): Remapping of vertices and triangles is not necessary when we do recycling (i.e during all updates between start and end stroke)
			{
				triIdx = mesh.GetNewTriIdx(triIdx);	// Remap element
				remapOccured = true;
			}
			++i;
		}
	}
	for(unsigned int i = 0; i < _verticesIdx.size();)
	{
		unsigned int& vtxIdx = _verticesIdx[i];
		if(mesh.IsVertexToBeRemoved(vtxIdx))
		{	// Remove element
			vtxIdx = _verticesIdx.back();
			_verticesIdx.pop_back();
		}
		else
		{
			if(doRemapping && mesh.VtxHasToMove(vtxIdx))	// Do remapping if not asked (save memory load access costs): Remapping of vertices and triangles is not necessary when we do recycling (i.e during all updates between start and end stroke)
			{
				vtxIdx = mesh.GetNewVtxIdx(vtxIdx);	// Remap element
				remapOccured = true;
			}
			++i;
		}
	}
	// If something has changed, we have to recompute bbox and rebuild submesh
	if(remapOccured || (initialVerticesIdxSize != _verticesIdx.size()) || (initialTrianglesIdxSize != _trianglesIdx.size()))
	{
		AddStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH);
		AddStateFlagsUpToRoot(CELL_STATE_HASTO_RECOMPUTE_BBOX);
	}
}
