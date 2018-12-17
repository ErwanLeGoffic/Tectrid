#pragma once

#define UNITYPLUGIN_API __declspec(dllexport) 

extern "C"
{
	UNITYPLUGIN_API void SculptEngine_SetMirrorMode(bool value);
	UNITYPLUGIN_API bool SculptEngine_IsMirrorModeActivated();
	UNITYPLUGIN_API bool SculptEngine_HasExpired();
	UNITYPLUGIN_API char const* SculptEngine_GetExpirationDate();

	UNITYPLUGIN_API void* GenBox_Generate(float wdith, float height, float depth);	// return Mesh pointer;
	UNITYPLUGIN_API void* GenSphere_Generate(float radius);
	UNITYPLUGIN_API void* Mesh_Clone(void* mesh);
	UNITYPLUGIN_API void* Mesh_Create(int* triangles, unsigned int triangleCount, float* vertices, unsigned int vertexCount);
	UNITYPLUGIN_API void* MeshLoader_LoadFromFile(char *filepath);
	UNITYPLUGIN_API void* MeshLoader_LoadFromTextBuffer(char *fileData, char *filename);
	UNITYPLUGIN_API bool MeshRecorder_Save(char *filepath, void *mesh);
	UNITYPLUGIN_API void Mesh_Delete(void *mesh);
	UNITYPLUGIN_API bool Mesh_IsManifold(void *mesh);

	UNITYPLUGIN_API unsigned int Mesh_GetID(void *mesh);
	UNITYPLUGIN_API void Mesh_GetBBox(void *mesh, float *min, float *max);
	UNITYPLUGIN_API unsigned int Mesh_GetNbTriangles(void *mesh);
	UNITYPLUGIN_API unsigned int Mesh_GetNbVertices(void *mesh);
	UNITYPLUGIN_API void Mesh_FillTriangles(void *mesh, int* data);
	UNITYPLUGIN_API void Mesh_FillVertices(void *mesh, float* data);
	UNITYPLUGIN_API void Mesh_FillNormals(void *mesh, float* data);

	UNITYPLUGIN_API bool Mesh_GetClosestIntersectionPoint(void *mesh, float* meshRotAndScale3x3Matrix, float* meshPosition, float* ray, float* intersectionPoint, float* intersectionNormal);

	UNITYPLUGIN_API bool Mesh_CanUndo(void *mesh);
	UNITYPLUGIN_API bool Mesh_Undo(void *mesh);
	UNITYPLUGIN_API bool Mesh_CanRedo(void *mesh);
	UNITYPLUGIN_API bool Mesh_Redo(void *mesh);

	UNITYPLUGIN_API void* BrushDraw_Create(void *mesh);
	UNITYPLUGIN_API void* BrushInflate_Create(void *mesh);
	UNITYPLUGIN_API void* BrushFlatten_Create(void *mesh);
	UNITYPLUGIN_API void* BrushDrag_Create(void *mesh);
	UNITYPLUGIN_API void* BrushDig_Create(void *mesh);
	UNITYPLUGIN_API void* BrushCADDrag_Create(void *mesh);
	UNITYPLUGIN_API void Brush_StartStroke(void *brush);
	UNITYPLUGIN_API void Brush_UpdateStroke(void *brush, float* meshRotAndScale3x3Matrix, float* meshPosition, float* rayOrigin, float* rayDirection, float rayLength, float radius, float effectRatio);
	UNITYPLUGIN_API void Brush_EndStroke(void *brush);

	UNITYPLUGIN_API void Brush_Delete(void *brush);

	// Sub meshes
	UNITYPLUGIN_API void Mesh_UpdateSubMeshes(void *fullMesh);
	UNITYPLUGIN_API unsigned int Mesh_GetSubMeshCount(void *fullMesh);
	UNITYPLUGIN_API void* Mesh_GetSubMesh(void *fullMesh, unsigned int index);
	UNITYPLUGIN_API bool Mesh_IsSubMeshExist(void *fullMesh, unsigned int submeshID);

	UNITYPLUGIN_API unsigned int SubMesh_GetID(void *subMesh);
	UNITYPLUGIN_API unsigned int SubMesh_GetVersionNumber(void *subMesh);
	UNITYPLUGIN_API void SubMesh_GetBBox(void *subMesh, float *min, float *max);
	UNITYPLUGIN_API unsigned int SubMesh_GetNbTriangles(void *subMesh);
	UNITYPLUGIN_API unsigned int SubMesh_GetNbVertices(void *subMesh);
	UNITYPLUGIN_API void SubMesh_FillTriangles(void *subMesh, int* data);
	UNITYPLUGIN_API void SubMesh_FillVertices(void *subMesh, float* data);
	UNITYPLUGIN_API void SubMesh_FillNormals(void *subMesh, float* data);

	// CSG / Boolean meshes
	UNITYPLUGIN_API bool CSGMerge(void *mesh, void *otherMesh, float* rotAndScale3x3Matrix, float* position);
	UNITYPLUGIN_API bool CSGSubtract(void *mesh, void *otherMesh, float* rotAndScale3x3Matrix, float* position);
	UNITYPLUGIN_API bool CSGIntersect(void *mesh, void *otherMesh, float* rotAndScale3x3Matrix, float* position);
}