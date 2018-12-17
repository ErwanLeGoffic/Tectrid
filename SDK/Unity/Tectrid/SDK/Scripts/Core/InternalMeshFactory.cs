using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using Tectrid.Primitives;
using UnityEngine;

namespace Tectrid
{
    public class InternalMeshFactory
    {

        /* public static IntPtr BuildInternalMesh(EditableMeshCreationInfos creationInfos)
        {
            IntPtr internalMesh = IntPtr.Zero;

            switch (creationInfos._currentCreationType)
            {
                case EditableMeshCreationInfos.CreationType.FromFile:
                    internalMesh = BuildInternalMeshFromBinaryData(creationInfos.FileBinaryData, creationInfos.FilePath);
                    break;
                case EditableMeshCreationInfos.CreationType.MeshCopy:
                    internalMesh = BuildInternalMesh(creationInfos.MeshToCopy);
                    break;
                case EditableMeshCreationInfos.CreationType.ObjectMeshSubstitution:
                    internalMesh = BuildInternalMesh(creationInfos.AttachedObject);
                    break;
                case EditableMeshCreationInfos.CreationType.Primitive:
                        internalMesh = BuildInternalMeshFromPrimitiveInfos(creationInfos.PrimitiveType, creationInfos.PrimitiveParameters);
                    break;
            }
            if (!IsReady(internalMesh) || !IsManifold(internalMesh))
                return IntPtr.Zero;
            return internalMesh;
        } */

        public static IntPtr BuildInternalMesh(IntPtr intPtr)
        {
            Debug.Log("Building by cloning");
            return DLL.Mesh_Clone(intPtr);
        }

        public static IntPtr BuildInternalMesh(string filePath)
        {
            Debug.Log("Building mesh with : " + filePath);
            if (filePath == string.Empty)
                filePath = null;
            return DLL.MeshLoader_LoadFromFile(filePath);
        }

        public static IntPtr BuildInternalMesh(GameObject gameObject)
        {
            MeshFilter meshFilter = gameObject.GetComponent<MeshFilter>();
            if (meshFilter == null || meshFilter.sharedMesh == null)
                return IntPtr.Zero;
            return BuildInternalMesh(meshFilter.sharedMesh);
        }

        public static IntPtr BuildInternalMesh(Mesh unityMesh)
        {
            GCHandle gcTriangles = GCHandle.Alloc(unityMesh.triangles, GCHandleType.Pinned);
            GCHandle gcVertices = GCHandle.Alloc(unityMesh.vertices, GCHandleType.Pinned);
            IntPtr internalMesh = DLL.Mesh_Create(gcTriangles.AddrOfPinnedObject(), unityMesh.triangles.Length / 3, gcVertices.AddrOfPinnedObject(), unityMesh.vertices.Length);
            gcTriangles.Free();
            gcVertices.Free();
            return internalMesh;
        }

        public static IntPtr BuildInternalMesh(Primitive primitive)
        {
            return primitive.Generate();
        }

        public static IntPtr BuildInternalMesh(Primitive.Types primitiveType, List<float> primitiveParameters)
        {
            return InternalPrimitiveFactory.BuildInternalPrimitive(primitiveType, primitiveParameters);
        }

        public static IntPtr BuildInternalMesh(string binaryData, string filePath)
        {
            return DLL.MeshLoader_LoadFromTextBuffer(binaryData, filePath);
        }

        #region Error Checking

        private static bool IsReady(IntPtr internalMesh)
        {
            return internalMesh != IntPtr.Zero;
        }

        private static bool IsManifold(IntPtr internalMesh)
        {
            return DLL.Mesh_IsManifold(internalMesh);
        }

        #endregion
    }
}
