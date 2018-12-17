#include "MeshRecorder.h"
#include "Mesh.h"
#include "SculptEngine.h"
#include <fstream>
#include <algorithm>

const char *MeshRecorder::tectridBinaryHeader = "TectridBinary";

const char *MeshRecorder::GetFilenameExt(const char *filename)
{
	const char *dot = strrchr(filename, '.');
	if(!dot || dot == filename) return "";
	return dot + 1;
}

bool MeshRecorder::SaveToFile(Mesh const& mesh, std::string const& filename)
{
	std::string fileExt(MeshRecorder::GetFilenameExt(filename.c_str()));
	std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);	// Convert to lowercase
	std::ofstream outputFileStream(filename, std::ofstream::binary | std::ofstream::trunc);
	if(!outputFileStream)
	{
		fprintf(stderr, "Cannot open %s", filename.c_str());
		return false;
	}
	std::stringstream outputStringStream;
	bool saveToBufferOk = SaveData(mesh, fileExt, outputStringStream);
	if(saveToBufferOk)
	{
		outputFileStream << outputStringStream.str();
		return true;
	}
	return false;
}

std::string MeshRecorder::SaveToTextBuffer(Mesh const& mesh, std::string const& fileExt)
{
	std::stringstream outputStringStream;
	std::string lowercaseFileExt;
	std::transform(fileExt.begin(), fileExt.end(), lowercaseFileExt.begin(), ::tolower);	// Convert to lowercase
	bool saveToBufferOk = SaveData(mesh, fileExt, outputStringStream);
	if(saveToBufferOk)
		return std::move(outputStringStream.str());
	return std::string();	// Empty string
}

bool MeshRecorder::SaveData(Mesh const& mesh, std::string const& fileExt, std::stringstream& outputStream)
{
	std::vector<unsigned int> triangles = mesh.GetTriangles();	// Copy triangle indices
	if(SculptEngine::IsTriangleOrientationInverted())
	{	// Flip all triangles if 3D engine needs all to be inverted
		for(unsigned int i = 0; i < triangles.size(); i += 3)
		{
			unsigned int swap = triangles[i];
			triangles[i] = triangles[i + 2];
			triangles[i + 2] = swap;
		}
	}
	if(fileExt == "obj")
	{	// OBJ File
		for(Vector3 vertex : mesh.GetVertices())
		{
			_transformMatrix.Transform(vertex);
			outputStream << "v " << vertex.x << " " << vertex.y << " " << vertex.z << std::endl;
		}
		for(Vector3 normal : mesh.GetNormals())
		{
			_transformMatrix.Transform(normal);
			normal.Normalize();
			outputStream << "vn " << normal.x << " " << normal.y << " " << normal.z << std::endl;
		}
		unsigned int vtxCount = 0;
		for(unsigned int vtxIndex : triangles)
		{
			if((vtxCount % 3) == 0)
				outputStream << "f ";
			outputStream << vtxIndex + 1 << "//" << vtxIndex + 1 << " ";
			if((vtxCount++ % 3) == 2)
				outputStream << std::endl;
		}
	}
	else if(fileExt == "stl")
	{	// STL binary File
		// Write the STL 80 byte header (leave it empty)
		for(int i = 0; i < 80; ++i)
			outputStream.put(0);
		// Save data
		unsigned int nbTriangles = (unsigned int) triangles.size() / 3;
		outputStream.write((const char*) &nbTriangles, sizeof(unsigned int));
		std::vector<Vector3> vertices = mesh.GetVertices();	// Copy vector to be able to transform vertices without modifying input mesh
		for(Vector3& vertex : vertices)
			_transformMatrix.Transform(vertex);
		std::vector<Vector3> const& triNormals = mesh.GetTrisNormal();
		for(int i = 0; i < triangles.size(); i += 3)
		{
			outputStream.write((const char*) &triNormals[i / 3], sizeof(Vector3));
			outputStream.write((const char*) &vertices[triangles[i]], sizeof(Vector3));
			outputStream.write((const char*) &vertices[triangles[i + 1]], sizeof(Vector3));
			outputStream.write((const char*) &vertices[triangles[i + 2]], sizeof(Vector3));
			outputStream.put(0);
			outputStream.put(0);
		}
	}
	else if(fileExt == "tct")
	{	// Tectrid binary format
		// Write a header
		outputStream << MeshRecorder::tectridBinaryHeader;
		// Vertices (don't serialize normals to save space)
		unsigned int nbVertices = (unsigned int) mesh.GetVertices().size();
		outputStream.write((const char*) &nbVertices, sizeof(unsigned int));
		for(Vector3 vertex : mesh.GetVertices())
		{
			_transformMatrix.Transform(vertex);
			outputStream.write((const char*) &vertex, sizeof(Vector3));
		}
		/*for(Vector3 normal : mesh.GetNormals())
		outputStream.write((const char*) &normal, sizeof(Vector3));*/
		// Triangles
		unsigned int nbTriangles = (unsigned int) triangles.size() / 3;
		outputStream.write((const char*) &nbTriangles, sizeof(unsigned int));
		for(unsigned int vtxIndex : triangles)
			outputStream.write((const char*) &vtxIndex, sizeof(unsigned int));
	}
	else
	{
		printf("unrecognized file extension");
		return false;
	}
	return true;
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(MeshRecorder)
{
	class_<MeshRecorder>("MeshRecorder")
		.constructor<>()
		.function("SaveToTextBuffer", &MeshRecorder::SaveToTextBuffer, allow_raw_pointers())
		.function("SetTransformMatrix", &MeshRecorder::SetTransformMatrix);
}
#endif // __EMSCRIPTEN__
