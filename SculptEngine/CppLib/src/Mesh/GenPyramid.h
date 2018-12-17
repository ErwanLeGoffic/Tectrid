#ifndef _GEN_PYRAMID_H_
#define _GEN_PYRAMID_H_

#include <vector>
#include "Math\Math.h"

class Mesh;

class GenPyramid
{
public:
#ifdef LOW_DEF
	GenPyramid() : _nbVertexPerLine(25) {}
#elif defined(MEDIUM_DEF)
	GenPyramid() : _nbVertexPerLine(50) {}
#else
	GenPyramid() : _nbVertexPerLine(100) {}
#endif // LOW_DEF / MEDIUM_DEF
	Mesh* Generate(float width, float height, float depth);

private:
	void Gentriangles();
	void GenVertices(float width, float height, float depth);

private:
	const int _nbVertexPerLine;
	std::vector<Vector3> _vertices;
	std::vector<unsigned int> _triangles;
};

#endif // _GEN_PYRAMID_H_