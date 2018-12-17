#ifndef _GEN_SPHERE_H_
#define _GEN_SPHERE_H_

#include <vector>
#include "Math\Math.h"

class Mesh;

class GenSphere
{
public:
#ifdef LOW_DEF
	GenSphere() : _nbVertexPerLine(25) {}
#elif defined(MEDIUM_DEF)
	GenSphere() : _nbVertexPerLine(50) {}
#else
	GenSphere() : _nbVertexPerLine(100) {}
#endif // LOW_DEF / MEDIUM_DEF
	Mesh* Generate(float radius);

private:
	void Gentriangles();
	void GenVertices(float radius);

private:
	const int _nbVertexPerLine;
	std::vector<Vector3> _vertices;
	std::vector<unsigned int> _triangles;
};

#endif // _GEN_SPHERE_H_