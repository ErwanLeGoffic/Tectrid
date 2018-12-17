#include "GenCylinder.h"
#include "Mesh.h"
#include "SculptEngine.h"

Mesh* GenCylinder::Generate(float height, float radius)
{
	GenVertices(height, radius);
	Gentriangles();
	Mesh* mesh = new Mesh(_triangles, _vertices, -1, true, false, true, true, true);
	return mesh;
}

void GenCylinder::GenVertices(float height, float radius)
{
	float sideLength = 1;
	int nbVtxPerFace = _nbVertexPerLine * _nbVertexPerLine;
	_vertices.resize(nbVtxPerFace * 6);
	float absLimit = sideLength / 2.0f;
	float step = sideLength / (float) (_nbVertexPerLine - 1);

	for(int i = 0; i < _nbVertexPerLine; ++i)
	{
		for(int j = 0; j < _nbVertexPerLine; ++j)
		{
			int index = i * _nbVertexPerLine + j;
			float coo1 = -absLimit + step * i;
			float coo2 = -absLimit + step * j;
			Vector3 vtx(coo1, coo2, absLimit);
			// Face 1
			_vertices[index].x = vtx.x;
			_vertices[index].y = vtx.y;
			_vertices[index].z = -vtx.z;
			// Face 2
			index += nbVtxPerFace;
			_vertices[index].x = vtx.x;
			_vertices[index].y = vtx.y;
			_vertices[index].z = vtx.z;
			// Face 3
			index += nbVtxPerFace;
			_vertices[index].x = vtx.x;
			_vertices[index].y = vtx.z;
			_vertices[index].z = vtx.y;
			// Face 4
			index += nbVtxPerFace;
			_vertices[index].x = vtx.x;
			_vertices[index].y = -vtx.z;
			_vertices[index].z = vtx.y;
			// Face 5
			index += nbVtxPerFace;
			_vertices[index].x = -vtx.z;
			_vertices[index].y = vtx.x;
			_vertices[index].z = vtx.y;
			// Face 6
			index += nbVtxPerFace;
			_vertices[index].x = vtx.z;
			_vertices[index].y = vtx.x;
			_vertices[index].z = vtx.y;
		}
	}
	Vector3 scale(radius * 2.0f, radius * 2.0f, height);
	for(Vector3& vertex : _vertices)
	{
		// transform cube into box
		vertex *= scale;
		// Transform box into cylinder
		Vector3 flatVertex = vertex;
		flatVertex.z = 0.0f;
		float length = flatVertex.Length();
		if(fabs(vertex.z) < (height / 2.0f))
		{	// Cylindric part
			if(length != 0.0f)
			{
				flatVertex /= length;
				flatVertex *= radius;
			}
		}
		else
		{	// Top circle parts (transform tessellated square into circle)
			if(length != 0.0f)
			{
				flatVertex *= max(fabsf(flatVertex.x), fabsf(flatVertex.y)) / radius;
				flatVertex /= length;
				flatVertex *= radius;
			}
		}
		vertex.x = flatVertex.x;
		vertex.y = flatVertex.y;
	}	
}

void GenCylinder::Gentriangles()
{
	int nbVtxPerFace = _nbVertexPerLine * _nbVertexPerLine;
	int nbTriPerFace = 3 * 2 * ((_nbVertexPerLine - 1) * (_nbVertexPerLine - 1));
	_triangles.resize(6 * nbTriPerFace);
	int flipEvenOdd = SculptEngine::IsTriangleOrientationInverted() ? 0 : 1;
	for(int i = 0; i < _nbVertexPerLine - 1; ++i)
	{
		for(int j = 0; j < _nbVertexPerLine - 1; ++j)
		{
			for(int k = 0; k < 6; ++k)
			{
				int index = (3 * 2 * (i * (_nbVertexPerLine - 1) + j)) + k * nbTriPerFace;
				int vtx1 = i * _nbVertexPerLine + j + k * nbVtxPerFace;
				int vtx2 = vtx1 + 1;
				int vtx3 = (i + 1) * _nbVertexPerLine + j + k * nbVtxPerFace;
				int vtx4 = vtx3 + 1;
				if(k % 2 == flipEvenOdd)
				{   // Flip triangles one over two face
					++vtx1; --vtx2; ++vtx3; --vtx4;
				}
				// First quad's triangle
				_triangles[index++] = vtx1;
				_triangles[index++] = vtx2;
				_triangles[index++] = vtx3;
				// Second quad's triangle
				_triangles[index++] = vtx3;
				_triangles[index++] = vtx2;
				_triangles[index++] = vtx4;
			}
		}
	}
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(GenCylinder)
{
	class_<GenCylinder>("GenCylinder")
		.constructor<>()
		.function("Generate", &GenCylinder::Generate, allow_raw_pointers());
}
#endif // __EMSCRIPTEN__
