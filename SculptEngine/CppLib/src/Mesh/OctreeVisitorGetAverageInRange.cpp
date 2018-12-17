#include "OctreeVisitorGetAverageInRange.h"
#include "Octree.h"
#include "Mesh.h"

bool VisitorGetAverageInRange::HasToVisit(OctreeCell& cell)
{
	return _rangeSphere.Intersects(cell.GetContentBBox());
}

void VisitorGetAverageInRange::VisitEnter(OctreeCell& cell)
{
	if(!cell.GetTrianglesIdx().empty() || !cell.GetVerticesIdx().empty())
	{
		std::vector<Vector3> const& vertices = _mesh.GetVertices();
		std::vector<Vector3> const& normals = _mesh.GetNormals();
		for(unsigned int const& vtxIdx : cell.GetVerticesIdx())
		{
			Vector3 const& pos = vertices[vtxIdx];
			if(pos.DistanceSquared(_rangeSphere.GetCenter()) < _rangeRadiusSquared)
			{
				_verticesAccumulator += pos;
				_normalsAccumulator += normals[vtxIdx];
				++_nbVerticesAccumulated;
			}
		}
	}
}
