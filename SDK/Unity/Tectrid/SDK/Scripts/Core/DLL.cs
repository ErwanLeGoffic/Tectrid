using System;
using System.Runtime.InteropServices;

namespace Tectrid
{
    public class DLL
    {
        [DllImport("TectridSDK")]
        static extern public void SculptEngine_SetMirrorMode(bool value);
        [DllImport("TectridSDK")]
        static extern public bool SculptEngine_IsMirrorModeActivated();

        [DllImport("TectridSDK")]
        static extern public IntPtr GenBox_Generate(float width, float height, float depth);
        [DllImport("TectridSDK")]
        static extern public IntPtr GenSphere_Generate(float radius);
        [DllImport("TectridSDK")]
        static extern public IntPtr Mesh_Create(IntPtr triangles, int triangleCount, IntPtr vertices, int vertexCount);
        [DllImport("TectridSDK")]
		static extern public IntPtr MeshLoader_LoadFromFile(string filepath);
        [DllImport("TectridSDK")]
        static extern public IntPtr MeshLoader_LoadFromTextBuffer(string fileData, string filename);
        [DllImport("TectridSDK")]
        static extern public bool MeshRecorder_Save(string filepath, IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public IntPtr Mesh_Clone(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public IntPtr Mesh_Delete(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public bool Mesh_IsManifold(IntPtr mesh);

        [DllImport("TectridSDK")]
        static extern public uint Mesh_GetID(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public void Mesh_GetBBox(IntPtr mesh, IntPtr min, IntPtr max);
        [DllImport("TectridSDK")]
        static extern public uint Mesh_GetNbTriangles(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public uint Mesh_GetNbVertices(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public void Mesh_FillTriangles(IntPtr mesh, IntPtr data);
        [DllImport("TectridSDK")]
        static extern public void Mesh_FillVertices(IntPtr mesh, IntPtr data);
        [DllImport("TectridSDK")]
        static extern public void Mesh_FillNormals(IntPtr mesh, IntPtr data);

        [DllImport("TectridSDK")]
        static extern public bool Mesh_CanUndo(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public bool Mesh_Undo(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public bool Mesh_CanRedo(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public bool Mesh_Redo(IntPtr mesh);

        [DllImport("TectridSDK")]
        static extern public bool Mesh_GetClosestIntersectionPoint(IntPtr mesh, IntPtr meshRotAndScale3x3Matrix, IntPtr meshPosition, IntPtr ray, IntPtr intersectionPoint, IntPtr intersectionNormal);

        [DllImport("TectridSDK")]
        static extern public IntPtr BrushDraw_Create(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public IntPtr BrushInflate_Create(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public IntPtr BrushFlatten_Create(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public IntPtr BrushDrag_Create(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public IntPtr BrushDig_Create(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public IntPtr BrushCADDrag_Create(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public void Brush_StartStroke(IntPtr brush);
        [DllImport("TectridSDK")]
        static extern public void Brush_UpdateStroke(IntPtr brush, IntPtr meshRotAndScale3x3Matrix, IntPtr meshPosition, IntPtr rayOrigin, IntPtr rayDirection, float rayLength, float radius, float strengthRatio);
        [DllImport("TectridSDK")]
        static extern public void Brush_EndStroke(IntPtr brush);

        [DllImport("TectridSDK")]
        static extern public IntPtr Brush_Delete(IntPtr brush);

        // Sub meshes
        [DllImport("TectridSDK")]
        static extern public void Mesh_UpdateSubMeshes(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public uint Mesh_GetSubMeshCount(IntPtr mesh);
        [DllImport("TectridSDK")]
        static extern public IntPtr Mesh_GetSubMesh(IntPtr mesh, uint index);
        [DllImport("TectridSDK")]
        static extern public bool Mesh_IsSubMeshExist(IntPtr mesh, uint submeshID);

        [DllImport("TectridSDK")]
        static extern public uint SubMesh_GetID(IntPtr subMesh);
        [DllImport("TectridSDK")]
        static extern public uint SubMesh_GetVersionNumber(IntPtr subMesh);
        [DllImport("TectridSDK")]
        static extern public void SubMesh_GetBBox(IntPtr subMesh, IntPtr min, IntPtr max);
        [DllImport("TectridSDK")]
        static extern public uint SubMesh_GetNbTriangles(IntPtr subMesh);
        [DllImport("TectridSDK")]
        static extern public uint SubMesh_GetNbVertices(IntPtr subMesh);
        [DllImport("TectridSDK")]
        static extern public void SubMesh_FillTriangles(IntPtr subMesh, IntPtr data);
        [DllImport("TectridSDK")]
        static extern public void SubMesh_FillVertices(IntPtr subMesh, IntPtr data);
        [DllImport("TectridSDK")]
        static extern public void SubMesh_FillNormals(IntPtr subMesh, IntPtr data);

        // CSG / Boolean meshes
        [DllImport("TectridSDK")]
        static extern public bool CSGMerge(IntPtr mesh, IntPtr otherMesh, IntPtr rotAndScale3x3Matrix, IntPtr position);
        [DllImport("TectridSDK")]
        static extern public bool CSGSubtract(IntPtr mesh, IntPtr otherMesh, IntPtr rotAndScale3x3Matrix, IntPtr position);
        [DllImport("TectridSDK")]
        static extern public bool CSGIntersect(IntPtr mesh, IntPtr otherMesh, IntPtr rotAndScale3x3Matrix, IntPtr position);
    }
}
