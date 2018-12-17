#include "MeshLoader.h"
#include "MeshRecorder.h"
#include "Mesh.h"
#include "SculptEngine.h"
#include <fstream>
#include <algorithm>

Mesh* MeshLoader::LoadFromFile(std::string const& filename)
{
	bool loasSucceed = false;
	{ // Block to free all file content memory (ifstream and stringstream)
		// Open file
		std::ios::openmode openMode;
		openMode = std::ios::binary;	// Even on text files, use binary. We get a byte buffer and then STL will do the job.
		std::ifstream inputFile(filename, openMode);
		if(!inputFile)
		{
			fprintf(stderr, "Cannot open %s", filename.c_str());
			return nullptr;
		}
		// Get file extension
		std::string fileExt(MeshRecorder::GetFilenameExt(filename.c_str()));
		std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);	// Convert to lowercase
		// Stream the whole content into memory (into a string streamer and eventually extract data)
		std::stringstream inputFileData;
		inputFileData << inputFile.rdbuf();
		loasSucceed = LoadData(inputFileData, fileExt);
	}
	if(loasSucceed)
	{
		Mesh* mesh = new Mesh(_triangles, _vertices, -1, true, true, true, true, true);
		return mesh;
	}
	else
		return nullptr;
}

Mesh* MeshLoader::LoadFromTextBuffer(std::string const& fileData, std::string const& filename)
{
	bool loasSucceed = false;
	{ // Block to free stringstream memory
		// Get file extension
		std::string fileExt(MeshRecorder::GetFilenameExt(filename.c_str()));
		std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);	// Convert to lowercase
		// Stream the whole content into memory (into a string streamer and eventually extract data)
		std::stringstream inputFileData;
		inputFileData << fileData;
		loasSucceed = LoadData(inputFileData, fileExt);
	}
	if(loasSucceed)
	{
		Mesh* mesh = new Mesh(_triangles, _vertices, -1, true, true, true, true, true);
		return mesh;
	}
	else
		return nullptr;
}

bool MeshLoader::LoadData(std::stringstream& fileTextData, std::string const& fileExt)
{
	if(fileExt == "tct")
	{
		if(fileTextData.str().substr(0, strlen(MeshRecorder::tectridBinaryHeader)) == MeshRecorder::tectridBinaryHeader)
		{	// Tectrid's binary format
			fileTextData.seekg(strlen(MeshRecorder::tectridBinaryHeader));
			unsigned int nbVertices = 0;
			fileTextData.read((char*) &nbVertices, sizeof(nbVertices));
			_vertices.resize(nbVertices);
			for(unsigned int i = 0; i < nbVertices; ++i)
			{
				if(!fileTextData.good())
					return false;
				fileTextData.read((char*) &_vertices[i], sizeof(Vector3));
			}
			unsigned int nbTriangles = 0;
			fileTextData.read((char*) &nbTriangles, sizeof(nbTriangles));
			_triangles.resize(nbTriangles * 3);
			for(unsigned int i = 0; i < nbTriangles * 3; ++i)
			{
				if(!fileTextData.good())
					return false;
				fileTextData.read((char*) &_triangles[i], sizeof(unsigned int));
			}
		}
		else
			return false;
	}
	else if(fileExt == "obj")
	{	// Obj text format
		std::string line;
		while(std::getline(fileTextData, line))
		{
			if(line.substr(0, 2) == "v ")
			{
				std::istringstream s(line.substr(2));
				Vector3 v;
				s >> v.x;
				s >> v.y;
				s >> v.z;
				_vertices.push_back(v);
			}
			else if(line.substr(0, 3) == "vn ")
			{
				/*std::istringstream s(line.substr(3));
				Vector3 v;
				s >> v.x;
				s >> v.y;
				s >> v.z;
				_normals.push_back(v);*/
			}
			else if(line.substr(0, 2) == "f ")
			{
				std::istringstream s(line.substr(2));
				std::string vtxTxtNorm;	// Each index could be "f vertex/texture_coordinate/normal". If there is no texture and normal, it's "f vertex"
				std::vector<unsigned int> indices;
				for(int i = 0; s >> vtxTxtNorm; ++i)
				{
					std::size_t pos = vtxTxtNorm.find("/");
					std::string vtx = vtxTxtNorm.substr(0, pos);
					int vtxIndex = std::stoi(vtx);
					indices.push_back(--vtxIndex);
				}
				if(indices.size() == 3) // Triangle
				{
					_triangles.push_back(indices[0]);
					_triangles.push_back(indices[1]);
					_triangles.push_back(indices[2]);
				}
				else if(indices.size() == 4) // Quad
				{
					_triangles.push_back(indices[0]);
					_triangles.push_back(indices[1]);
					_triangles.push_back(indices[2]);
					_triangles.push_back(indices[3]);
					_triangles.push_back(indices[0]);
					_triangles.push_back(indices[2]);
				}
				else // Don't know
				{
					// Do Nothing
				}
			}
			else if(line[0] == '#')
			{
				// Comment line ignored
			}
			else
			{
				// Line not handled
			}
		}
	}
	else if(fileExt == "stl")
	{
		if(fileTextData.str().substr(0, strlen("solid")) == "solid")
		{	// STL ASCII format
			std::string line;
			while(std::getline(fileTextData, line))
			{
				if(line.substr(0, 7) == "vertex ")
				{
					std::istringstream s(line.substr(7));
					Vector3 v;
					s >> v.x;
					s >> v.y;
					s >> v.z;
					_triangles.push_back((unsigned int) _vertices.size());
					_vertices.push_back(v);
				}
			}
		}
		else
		{	// STL binary format
			// Skip the 80 bytes header
			fileTextData.seekg(80);
			// Load data
			unsigned int nbTriangles = 0;
			fileTextData.read((char*) &nbTriangles, sizeof(nbTriangles));
			_triangles.resize(nbTriangles * 3);
			_vertices.resize(nbTriangles * 3);
			unsigned int curVtxIdx = 0;
			unsigned int curTriVtxIdx = 0;
			for(unsigned int i = 0; i < nbTriangles; ++i)
			{
				if(!fileTextData.good())
					return false;
				fileTextData.seekg(sizeof(Vector3), std::ios_base::cur);	// Skip triangle normal
				_triangles[curTriVtxIdx++] = curVtxIdx;
				fileTextData.read((char*) &_vertices[curVtxIdx++], sizeof(Vector3));
				_triangles[curTriVtxIdx++] = curVtxIdx;
				fileTextData.read((char*) &_vertices[curVtxIdx++], sizeof(Vector3));
				_triangles[curTriVtxIdx++] = curVtxIdx;
				fileTextData.read((char*) &_vertices[curVtxIdx++], sizeof(Vector3));
				fileTextData.seekg(2, std::ios_base::cur);	// Skip attributes
			}
		}
	}
	if(_triangles.empty() || _vertices.empty())
		return false;
	if(SculptEngine::IsTriangleOrientationInverted())
	{	// Flip all triangles if 3D engine needs all to be inverted
		for(unsigned int i = 0; i < _triangles.size(); i += 3)
		{
			unsigned int swap = _triangles[i];
			_triangles[i] = _triangles[i + 2];
			_triangles[i + 2] = swap;
		}
	}
	// Convert negative vertex indices
	for(unsigned int i = 0; i < _triangles.size(); ++i)
	{
		int triIdx = _triangles[i];
		if(triIdx < 0)
			_triangles[i] = (int) _vertices.size() + triIdx + 1;
	}

	printf("Obj file loaded, %d triangles, %d vertices\n", (int) _triangles.size() / 3, (int) _vertices.size());
	return true;
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(MeshLoader)
{
	class_<MeshLoader>("MeshLoader")
		.constructor<>()
		.function("LoadFromTextBuffer", &MeshLoader::LoadFromTextBuffer, allow_raw_pointers());
}
#endif // __EMSCRIPTEN__
