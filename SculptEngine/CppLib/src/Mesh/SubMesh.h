#ifndef _SUB_MESH_H_
#define _SUB_MESH_H_

#include <vector>
#include "Math\Math.h"
#include "Collisions\BBox.h"

#ifdef __EMSCRIPTEN__ 
#include <emscripten/val.h>
#endif // __EMSCRIPTEN__

class SubMesh
{
public:
	SubMesh(unsigned int id) : _id(id), _versionNumber(0) {}

	unsigned int GetID() const { return _id; }
	unsigned int GetVersionNumber() const { return _versionNumber; }
	void IncVersionNumber() { ++_versionNumber; }
	BBox const& GetBBox() const { return _BBox; }
	void SetBBox(BBox const& bbox) { ASSERT(bbox.IsValid()); _BBox = bbox; }

	std::vector<unsigned int> const& GetTriangles() const { return _triangles; }
	std::vector<Vector3> const& GetVertices() const { return _vertices; }
	std::vector<Vector3> const& GetNormals() const { return _normals; }

	std::vector<unsigned int>& GrabTriangles() { return _triangles; }
	std::vector<Vector3>& GrabVertices() { return _vertices; }
	std::vector<Vector3>& GrabNormals() { return _normals; }

#ifdef __EMSCRIPTEN__ 
	emscripten::val Triangles() { return emscripten::val(emscripten::typed_memory_view(_triangles.size() * sizeof(int), (char const *) _triangles.data())); }
	emscripten::val Vertices() { return emscripten::val(emscripten::typed_memory_view(_vertices.size() * sizeof(Vector3), (char const *) _vertices.data())); }
	emscripten::val Normals() { return emscripten::val(emscripten::typed_memory_view(_normals.size() * sizeof(Vector3), (char const *) _normals.data())); }
#endif // __EMSCRIPTEN__ 

private:
	// Display related
	std::vector<unsigned int> _triangles;
	std::vector<Vector3> _vertices;
	std::vector<Vector3> _normals;
	// Bbox
	BBox _BBox;
	// ID
	unsigned int _id;
	// Version number, to be able to trigger data update on the caller side
	unsigned int _versionNumber;
};

#endif // _SUB_MESH_H_