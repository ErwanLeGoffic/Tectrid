#ifndef _CSG_BSP_H_
#define _CSG_BSP_H_

#include <vector>
#include <memory>
#include "Math\Vector.h"
#include "Math\VectorDouble.h"
#include "Math\PlaneDouble.h"

// # class CSG_Bsp
// Holds a binary space partition tree representing a 3D solid. Two solids can
// be combined using the `union()`, `subtract()`, and `intersect()` methods.
class Csg_Bsp
{
public:

	// # class Vertex
	// Represents a vertex of a polygon. Use your own vertex class instead of this
	// one to provide additional features like texture coordinates and vertex
	// colors. Custom vertex classes need to provide a `pos` property and `clone()`,
	// `flip()`, and `interpolate()` methods that behave analogous to the ones
	// defined by `CSG.Vertex`.
	class Vertex
	{
	public:
		Vertex(Vector3 const& pos): _pos(double(pos.x), double(pos.y), double(pos.z)) {}
		Vertex(Vector3Double const& pos) : _pos(pos) {}
		Vertex Interpolate(Vertex const& other, double t) const
		{
			return Vertex(_pos.Lerp(other._pos, t));
		}
		Vector3Double const& GetPos() const { return _pos; }
		Vector3Double& GrabPos() { return _pos; }

	private:
		Vector3Double _pos;
	};

	// # class Plane
	// Represents a plane in 3D space.
	class Polygon;
	class Plane: public ::PlaneDouble
	{
	public:
		Plane(Vector3Double const& a, Vector3Double const& b, Vector3Double const& c): ::PlaneDouble(a, b, c) {}

		// Split `polygon` by this plane if needed, then put the polygon or polygon
		// fragments in the appropriate lists. Coplanar polygons go into either
		// `coplanarFront` or `coplanarBack` depending on their orientation with
		// respect to this plane. Polygons in front or in back of this plane go into
		// either `front` or `back`.
		void SplitPolygon(Polygon const& polygon, std::vector<Polygon>& coplanarFront, std::vector<Polygon>& coplanarBack, std::vector<Polygon>& front, std::vector<Polygon>& back);

	public:
		// `CSG.Plane.EPSILON` is the tolerance used by `splitPolygon()` to decide if a
		// point is on the plane.
		static constexpr double _EPSILON = 1e-8;
	};

	// # class Polygon
	// Represents a convex polygon. The vertices used to initialize a polygon must
	// be coplanar and form a convex loop. They do not have to be `CSG.Vertex`
	// instances but they must behave similarly (duck typing can be used for
	// customization).
	// 
	// Each convex polygon has a `shared` property, which is shared between all
	// polygons that are clones of each other or were split from the same polygon.
	// This can be used to define per-polygon properties (such as surface color).
	class Polygon
	{
	public:
		Polygon(std::vector<Vertex> const& vertices, unsigned int shared = 0) : _vertices(vertices), _shared(shared), _plane(vertices[0].GetPos(), vertices[1].GetPos(), vertices[2].GetPos())
		{
#ifdef _DEBUG
			for(Vertex vertex : vertices)
			{
				double dist = _plane.DistanceToVertex(vertex.GetPos());
				ASSERT(dist < Plane::_EPSILON);
			}
#endif // _DEBUG
		}
		std::vector<Vertex> const& GetVertices() const { return _vertices; }
		Plane const& GetPlane() const { return _plane; }
		unsigned int GetShared() const { return _shared; }
		void Flip()
		{
			std::reverse(_vertices.begin(), _vertices.end());
			_plane.Flip();
		}

	private:
		std::vector<Vertex> _vertices;
		unsigned int _shared;
		Plane _plane;
	};

	// # class Node
	// Holds a node in a BSP tree. A BSP tree is built from a collection of polygons
	// by picking a polygon to split along. That polygon (and all other coplanar
	// polygons) are added directly to that node and the other polygons are added to
	// the front and/or back subtrees. This is not a leafy BSP tree since there is
	// no distinction between internal and leaf nodes.
	class BspNode
	{
	public:
		BspNode() {}
		BspNode(std::vector<Polygon> const& polygons) { Build(polygons); }

		// Build a BSP tree out of `polygons`. When called on an existing tree, the
		// new polygons are filtered down to the bottom of the tree and become new
		// nodes there. Each set of polygons is partitioned using the first polygon
		// (no heuristic is used to pick a good split).
		void Build(std::vector<Polygon> inputPolygons);
		
		// Recursively remove all polygons in `polygons` that are inside this BSP tree.
		std::vector<Polygon> ClipPolygons(std::vector<Polygon>& inputPolygons);

		// Convert solid space to empty space and empty space to solid space.
		void Invert();

	private:
		std::unique_ptr<BspNode> _front;
		std::unique_ptr<BspNode> _back;
		std::unique_ptr<Plane> _plane;
	};

public:
	Csg_Bsp(std::vector<Polygon>& polygons, bool clearInput): _polygons(clearInput ? std::move(polygons) : polygons) {}
	std::vector<Polygon> const& GetPolygons() const { return _polygons; }

	Csg_Bsp Subtract(Csg_Bsp const& csg, bool inverThis, bool inverInput);

private:
	std::vector<Polygon> _polygons;
};

#endif // _CSG_BSP_H_