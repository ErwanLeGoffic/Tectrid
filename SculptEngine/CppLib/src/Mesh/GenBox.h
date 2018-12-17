#ifndef _GEN_BOX_H_
#define _GEN_BOX_H_

#include <vector>
#include "Math\Math.h"

class Mesh;

class GenBox
{
public:
#ifdef LOW_DEF
	GenBox(): _nbVertexPerLine(25) {}
#elif defined(MEDIUM_DEF)
	GenBox() : _nbVertexPerLine(50) {}
#else
	GenBox() : _nbVertexPerLine(100) {}
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

#endif // _GEN_BOX_H_