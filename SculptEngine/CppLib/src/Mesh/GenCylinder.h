#ifndef _GEN_CYLINDER_H_
#define _GEN_CYLINDER_H_

#include <vector>
#include "Math\Math.h"

class Mesh;

class GenCylinder
{
public:
#ifdef LOW_DEF
	GenCylinder() : _nbVertexPerLine(25) {}
#elif defined(MEDIUM_DEF)
	GenCylinder() : _nbVertexPerLine(50) {}
#else
	GenCylinder() : _nbVertexPerLine(100) {}
#endif // LOW_DEF / MEDIUM_DEF
	Mesh* Generate(float height, float radius);

private:
	void Gentriangles();
	void GenVertices(float height, float radius);

private:
	const int _nbVertexPerLine;
	std::vector<Vector3> _vertices;
	std::vector<unsigned int> _triangles;
};

#endif // _GEN_CYLINDER_H_