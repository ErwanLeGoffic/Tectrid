#ifndef _OCTREE_VISITOR_GETINTERSECTION_H_
#define _OCTREE_VISITOR_GETINTERSECTION_H_

#include <vector>
#include "OctreeVisitor.h"
#include "Mesh.h"

class VisitorGetIntersection : public OctreeVisitor
{
public:
	VisitorGetIntersection(Mesh const& mesh, Ray const& ray, bool cullBackFace, bool intersectTriToRemove) : OctreeVisitor(), _mesh(mesh), _ray(ray), _cullBackFace(cullBackFace), _intersectTriToRemove(intersectTriToRemove), _vertices(_mesh.GetVertices()), _triangles(_mesh.GetTriangles()), _trisBSphere(_mesh.GetTrisBSphere()) {}
	virtual bool HasToVisit(OctreeCell& cell);
	virtual void VisitEnter(OctreeCell& cell);
	virtual void VisitLeave(OctreeCell& /*cell*/) {}

	std::vector<float> const& GetIntersectionDists() const { return _intersectionDists; }
	std::vector<unsigned int> const& GetIntersectionTriangles() const { return _intersectionTriangles; }
	Vector3 const& GetIntersectionNormal(unsigned int index) const { return _mesh.GetTrisNormal()[_intersectionTriangles[index]]; }

private:
	Mesh const& _mesh;
	Ray const& _ray;
	bool _cullBackFace;
	bool _intersectTriToRemove;
	std::vector<float> _intersectionDists;
	std::vector<unsigned int> _intersectionTriangles;
	std::vector<Vector3> const& _vertices;
	std::vector<unsigned int> const& _triangles;
	std::vector<BSphere> const& _trisBSphere;
};

#endif // _OCTREE_VISITOR_GETINTERSECTION_H_