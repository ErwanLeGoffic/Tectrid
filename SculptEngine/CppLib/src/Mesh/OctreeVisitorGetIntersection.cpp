#include "OctreeVisitorGetIntersection.h"
#include "Octree.h"
#include "Mesh.h"

bool VisitorGetIntersection::HasToVisit(OctreeCell& cell)
{
	return _ray.Intersects(cell.GetContentBBox());
}

void VisitorGetIntersection::VisitEnter(OctreeCell& cell)
{
	if(!cell.GetTrianglesIdx().empty() || !cell.GetVerticesIdx().empty())
	{
		for(int triIndex : cell.GetTrianglesIdx())
		{
			if((!_mesh.IsTriangleToBeRemoved(triIndex) || _intersectTriToRemove) && _ray.Intersects(_trisBSphere[triIndex]))
			{
				float distance = -1;
				unsigned int const* vtxsIdx = &(_triangles[triIndex * 3]);
				if(_ray.Intersects(_vertices[vtxsIdx[0]], _vertices[vtxsIdx[1]], _vertices[vtxsIdx[2]], distance, _cullBackFace))
				{
					//DEBUG_intersectionPoints.push_back(ray.GetOrigin() + ray.GetDirection() * distance);
					_intersectionDists.push_back(distance);
					_intersectionTriangles.push_back(triIndex);
				}
			}
		}
	}
}
