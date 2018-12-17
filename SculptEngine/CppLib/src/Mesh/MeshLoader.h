#ifndef _MESH_LOADER_H_
#define _MESH_LOADER_H_

#include <vector>
#include "Math\Math.h"
#include <sstream>

class Mesh;

class MeshLoader
{
public:
	MeshLoader() {}
	Mesh* LoadFromFile(std::string const& filename);
	Mesh* LoadFromTextBuffer(std::string const& fileData, std::string const& filename);

private:
	bool LoadData(std::stringstream& fileTextData, std::string const& fileExt);

	std::vector<Vector3> _vertices;
	std::vector<unsigned int> _triangles;
};

#endif // _MESH_LOADER_H_