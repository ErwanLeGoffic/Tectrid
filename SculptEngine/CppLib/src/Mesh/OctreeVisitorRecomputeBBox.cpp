#include "OctreeVisitorRecomputeBBox.h"
#include "Mesh.h"
#include "Octree.h"

bool VisitorRecomputeBBox::HasToVisit(OctreeCell& /*cell*/)
{
	return true;
}

void VisitorRecomputeBBox::VisitLeave(OctreeCell& cell)
{
	if(!cell.TestStateFlags(CELL_STATE_HASTO_RECOMPUTE_BBOX) && !_forceAllCellRecomputing)
		return;
	cell.ClearStateFlags(CELL_STATE_HASTO_RECOMPUTE_BBOX);
	BBox& cellContentBBox = cell.GrabContentBBox();
	cellContentBBox.Reset();
	if(!cell.GetTrianglesIdx().empty() || !cell.GetVerticesIdx().empty())
	{	// Leaf node: compute bbox using tessellation data
		std::vector<Vector3> const& vertices = _mesh.GetVertices();
		std::vector<unsigned int> const& triangles = _mesh.GetTriangles();
		std::vector<unsigned int> const& cellTrianglesIdx = cell.GetTrianglesIdx();
		if(cellTrianglesIdx.empty())	// Take in account vertices in case of no triangle are fitting in the cell (sometimes it happens)
		{
			std::vector<unsigned int> const& cellVerticesIdx = cell.GetVerticesIdx();
			for(int const& vtxIdx : cellVerticesIdx)
				cellContentBBox.Encapsulate(vertices[vtxIdx]);
		}
		for(int const& i : cellTrianglesIdx)
		{
			unsigned int const* vtxsIdx = &(triangles[i * 3]);
			for(int j = 0; j < 3; ++j)
				cellContentBBox.Encapsulate(vertices[vtxsIdx[j]]);
		}
	}
	else
	{	// Non leaf node: compute bbox using children bbox
		ASSERT(cell.HasChildren());
		if(cell.HasChildren())
		{
			ASSERT(cell.GetTrianglesIdx().empty() && cell.GetVerticesIdx().empty());
			std::vector<std::unique_ptr<OctreeCell>> const& cellChildren = cell.GetChildren();
			for(std::unique_ptr<OctreeCell> const& childCell : cellChildren)
			{
				if(childCell != nullptr)
					cellContentBBox.Encapsulate(childCell->GetContentBBox());
			}
		}
	}
}