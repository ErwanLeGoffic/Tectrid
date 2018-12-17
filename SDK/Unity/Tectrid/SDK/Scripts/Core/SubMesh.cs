using UnityEngine;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Tectrid
{
    public class SubMesh
    {
        private int[] _triangles = null;
        private Vector3[] _vertices = null;
        private Vector3[] _normals = null;
        private GameObject _gameObject;
        private Mesh _mesh;
        private IntPtr _subMesh;
        private uint _ID;
        public uint ID { get { return _ID; } }
        private uint _versionNumber;

        public SubMesh(GameObject parentGameObject, IntPtr internalMesh, uint subMeshIndex, Material material)
        {

            // Create Unity Game object to hold the mesh onto
            _subMesh = DLL.Mesh_GetSubMesh(internalMesh, subMeshIndex);
            _ID = DLL.SubMesh_GetID(_subMesh);
            _versionNumber = DLL.SubMesh_GetVersionNumber(_subMesh);
            _gameObject = new GameObject();
            _gameObject.name = "SubMesh-" + _ID.ToString();
            _gameObject.transform.SetParent(parentGameObject.transform, false);
            MeshRenderer mr = _gameObject.AddComponent<MeshRenderer>();
            MeshFilter mf = _gameObject.AddComponent<MeshFilter>();
            mr.receiveShadows = false;
            mr.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;

            // Create unity Mesh
            _mesh = new Mesh();
            _mesh.name = "mesh";
            mr.material = material;
            mf.mesh = _mesh;
            BuildMeshData();
        }

        public void Cleanup()
        {
            UnityEngine.GameObject.Destroy(_gameObject);
        }

        public void Update()
        {
            uint curVersionNumber = DLL.SubMesh_GetVersionNumber(_subMesh);
            if (_versionNumber != curVersionNumber)  // Only update data if sub mesh has been updated
            {
                _versionNumber = curVersionNumber;
                BuildMeshData();
            }
        }

        public void GetBBox(ref UnityEngine.Bounds bbox)
        {
            GCHandle gcMin = GCHandle.Alloc(bbox.min, GCHandleType.Pinned);
            GCHandle gcMax = GCHandle.Alloc(bbox.max, GCHandleType.Pinned);
            DLL.SubMesh_GetBBox(_subMesh, gcMin.AddrOfPinnedObject(), gcMax.AddrOfPinnedObject());
            bbox.min = (Vector3)gcMin.Target;
            bbox.max = (Vector3)gcMax.Target;
            gcMin.Free();
            gcMax.Free();
        }

        private void BuildMeshData()
        {
            uint nbTris = DLL.SubMesh_GetNbTriangles(_subMesh);
            uint nbVtxs = DLL.SubMesh_GetNbVertices(_subMesh);
            _triangles = new int[nbTris];
            _vertices = new Vector3[nbVtxs];
            _normals = new Vector3[nbVtxs];
            GCHandle gcTriangles = GCHandle.Alloc(_triangles, GCHandleType.Pinned);
            GCHandle gcVertices = GCHandle.Alloc(_vertices, GCHandleType.Pinned);
            GCHandle gcNormals = GCHandle.Alloc(_normals, GCHandleType.Pinned);
            DLL.SubMesh_FillTriangles(_subMesh, gcTriangles.AddrOfPinnedObject());
            DLL.SubMesh_FillVertices(_subMesh, gcVertices.AddrOfPinnedObject());
            DLL.SubMesh_FillNormals(_subMesh, gcNormals.AddrOfPinnedObject());
            gcTriangles.Free();
            gcVertices.Free();
            gcNormals.Free();
            _mesh.Clear();
            _mesh.vertices = _vertices;
            _mesh.normals = _normals;
            _mesh.triangles = _triangles;
        }

        public void SetMaterial(Material material)
        {
            MeshRenderer mr = _gameObject.GetComponent<MeshRenderer>();
            if (mr != null)
                mr.material = material;
        }
    }
}
