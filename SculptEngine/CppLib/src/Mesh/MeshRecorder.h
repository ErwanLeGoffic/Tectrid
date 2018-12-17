#ifndef _MESH_RECORDER_H_
#define _MESH_RECORDER_H_

#include <vector>
#include "Math\Math.h"
#include "Math\Matrix.h"
#include <sstream>

class Mesh;

class MeshRecorder
{
public:
	MeshRecorder() {}
	bool SaveToFile(Mesh const& mesh, std::string const& filename);
	std::string SaveToTextBuffer(Mesh const& mesh, std::string const& fileExt);
	void SetTransformMatrix(Matrix3 const& transformMatrix) { _transformMatrix = transformMatrix; }
	static const char *GetFilenameExt(const char *filename);
	static const char *tectridBinaryHeader;

private:
	bool SaveData(Mesh const& mesh, std::string const& fileExt, std::stringstream& outputStream);

	Matrix3 _transformMatrix;
};

#endif // _MESH_RECORDER_H_