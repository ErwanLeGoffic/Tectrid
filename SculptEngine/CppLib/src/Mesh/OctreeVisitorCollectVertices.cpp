#include "OctreeVisitorCollectVertices.h"
#include "Octree.h"
#include "Mesh.h"
#include "ThicknessHandler.h"

OctreeVisitorCollectVertices::OctreeVisitorCollectVertices(Mesh const& mesh, Vector3 const& rangeCenterPoint, float rangeRadius, Vector3 const* selectDirection, float cosAngleLimit, bool dirtyOctreeCells, bool collectRejected, ThicknessHandler& thicknessHandler): OctreeVisitor(), _mesh(mesh), _rangeSphere(rangeCenterPoint, rangeRadius), _rangeRadiusSquared(rangeRadius * rangeRadius), _selectDirection(selectDirection), _dirtyOctreeCells(dirtyOctreeCells), _collectRejected(collectRejected), _cosAngleLimit(cosAngleLimit), _thicknessHandler(thicknessHandler)
{
	_collectedVertices.reserve(mesh.GetVertices().size());
	if(_collectRejected)
	{
		ASSERT(_selectDirection != nullptr);
		_rejectedVertices.reserve(mesh.GetVertices().size());
	}
}

bool OctreeVisitorCollectVertices::HasToVisit(OctreeCell& cell)
{
	return _rangeSphere.Intersects(cell.GetContentBBox());
}

void OctreeVisitorCollectVertices::VisitEnter(OctreeCell& cell)
{
	if(!cell.GetTrianglesIdx().empty() || !cell.GetVerticesIdx().empty())
	{
		if(_dirtyOctreeCells)
		{
			cell.AddStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH | CELL_STATE_HASTO_EXTRACT_OUTOFBOUNDS_GEOM);
			cell.AddStateFlagsUpToRoot(CELL_STATE_HASTO_RECOMPUTE_BBOX);
		}
		std::vector<Vector3> const& vertices = _mesh.GetVertices();
		if(_selectDirection != nullptr)
		{
			std::vector<Vector3> const& normals = _mesh.GetNormals();
			for(unsigned int const& vtxIdx : cell.GetVerticesIdx())
			{
				if(!_mesh.IsVertexToBeRemoved(vtxIdx) && vertices[vtxIdx].DistanceSquared(_rangeSphere.GetCenter()) < _rangeRadiusSquared)
				{
					if(_selectDirection->Dot(normals[vtxIdx]) >= _cosAngleLimit)	// Rejected if looking backwards from the select direction
						_collectedVertices.push_back(vtxIdx);
					else if(_collectRejected)
						_rejectedVertices.push_back(vtxIdx);
					_thicknessHandler.AddVertexToTestList(vtxIdx);
				}
			}
		}
		else
		{
			for(unsigned int const& vtxIdx : cell.GetVerticesIdx())
			{
				if(!_mesh.IsVertexToBeRemoved(vtxIdx) && vertices[vtxIdx].DistanceSquared(_rangeSphere.GetCenter()) < _rangeRadiusSquared)
				{
					_collectedVertices.push_back(vtxIdx);
					_thicknessHandler.AddVertexToTestList(vtxIdx);
				}
			}
		}
	}
}

Vector3 OctreeVisitorCollectVertices::ComputeAveragePosition() const
{
	Vector3 verticesAccumulator;
	unsigned int nbVerticesAccumulated = 0;
	std::vector<Vector3> const& vertices = _mesh.GetVertices();
	for(unsigned int vtxIdx : _collectedVertices)
	{
		verticesAccumulator += vertices[vtxIdx];
		++nbVerticesAccumulated;
	}
	if(nbVerticesAccumulated > 0)
		verticesAccumulator /= float(nbVerticesAccumulated);
	return verticesAccumulator;
}

Vector3 OctreeVisitorCollectVertices::ComputeAverageNormal() const
{
	Vector3 normalsAccumulator;
	unsigned int nbNormalsAccumulated = 0;
	std::vector<Vector3> const& normals = _mesh.GetNormals();
	for(unsigned int vtxIdx : _collectedVertices)
	{
		normalsAccumulator += normals[vtxIdx];
		++nbNormalsAccumulated;
	}
	return nbNormalsAccumulated == 0 ? Vector3(0, 1, 0) : (normalsAccumulator / float(nbNormalsAccumulated)).Normalized();
}