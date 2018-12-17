using UnityEngine;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Tectrid.Primitives;
using System.ComponentModel;

namespace Tectrid
{
    public class EditableMesh : MonoBehaviour
    {
        private Bounds _Bounds;

        public Bounds Bounds
        {
            get
            {
                if (transform.hasChanged)
                    UpdateBounds();
                return _Bounds;
            }
            private set
            {
                _Bounds = value;
            }
        }
        [SerializeField]
        public bool Editable = true;

        private MeshFilter MeshFilter
        {
            get
            {
                MeshFilter mf = transform.GetComponent<MeshFilter>();
                if (mf == null)
                    mf = gameObject.AddComponent<MeshFilter>();
                return mf;
            }
        }

        [SerializeField]
        private EditableMeshCreationInfos StartCreationInfos;

        private IntPtr _internalMesh = IntPtr.Zero;
        [SerializeField]
        private Dictionary<uint, SubMesh> _subMeshes = new Dictionary<uint, SubMesh>();
        private Dictionary<Brush.Type, Brush> _brushes = new Dictionary<Brush.Type, Brush>();
        [SerializeField] [HideInInspector]
        private Material _material;
        [SerializeField]
        private Brush _currentBrush;
        [SerializeField]
        private bool? _shattered;

        public Material MeshMaterial
        {
            get { return _material; }
            set
            {
                _material = value;
                if (_shattered == false)
                {
                    MeshRenderer mr = GetComponent<MeshRenderer>();
                    mr.material = _material;
                }
                else
                {
                    foreach (SubMesh subMesh in _subMeshes.Values)
                        subMesh.SetMaterial(_material);
                }
            }
        }
        private bool _InPlayMode = false;

        private void Awake()
        {
            _Bounds = new Bounds();
        }

        private void Start()
        {
            SceneManager.Instance.RegisterEditableMesh(this);
            if (StartCreationInfos != null)
                StartCreationInfos.BuildPreview(this);
            _InPlayMode = true;
        }

        private void OnApplicationQuit()
        {
            _InPlayMode = false;
        }

        #region Mesh Building

        /// <summary> Build Editable Mesh From File (.obj) </summary>
        public void Build(string path)
        {
            DeleteInternalMesh();
            _internalMesh = InternalMeshFactory.BuildInternalMesh(path);
            LoadMesh();
        }

        /// <summary> Build Editable Mesh From another Editable Mesh </summary>
        public void Build(IntPtr intPtr)
        {
            DeleteInternalMesh();
            _internalMesh = InternalMeshFactory.BuildInternalMesh(intPtr);
            Debug.Log("new mesh inner ptr : " + _internalMesh);
            LoadMesh();
        }

        /// <summary> Build Editable Mesh From any Unity Mesh </summary>
        public void Build(Mesh mesh)
        {
            DeleteInternalMesh();
            _internalMesh = InternalMeshFactory.BuildInternalMesh(mesh);
            LoadMesh();
        }

        /// <summary> Build Editable Mesh From the Unity Mesh currently present on Gameobject </summary>
        public void Build()
        {
            DeleteInternalMesh();
            _internalMesh = InternalMeshFactory.BuildInternalMesh(gameObject);
            LoadMesh();
        }

        ///<summary> Build Primitive Editable Mesh  </summary>
        /// <example> editableMesh.Build(new Tectrid.Primitives.Sphere(10)); </example>
        public void Build(Primitive primitive)
        {
            DeleteInternalMesh();
            _internalMesh = InternalMeshFactory.BuildInternalMesh(primitive);
            LoadMesh();
        }

        /// <summary> Build Editable Mesh From obj file data </summary>
        public void Build(string data, string filePath)
        {
            DeleteInternalMesh();
            _internalMesh = InternalMeshFactory.BuildInternalMesh(data, filePath);
            LoadMesh();
        }

        public void Unbuild()
        {
            if (_internalMesh != IntPtr.Zero)
            {
                MeshFilter.mesh = null;
                DeleteInternalMesh();
                ClearSubMeshes();
            }
        }

        #endregion // ! Mesh Building

        #region Init

        private void LoadMesh(bool update = true)
        {
            if (_internalMesh != IntPtr.Zero)
            {
                MeshRenderer meshRenderer = transform.GetComponent<MeshRenderer>();
                if (meshRenderer != null)
                    _material = meshRenderer.sharedMaterial;
                else
                    _material = CreateDefaultMaterial();
                InitBrushes();
                if (update)
                    UpdateEditableMesh();
            }
            else
                Debug.Log("failed to load");
        }

        private void InitBrushes()
        {
            _brushes.Clear();
            _brushes.Add(Brush.Type.Draw, new Brush(this, Brush.Type.Draw));
            _brushes.Add(Brush.Type.Dig, new Brush(this, Brush.Type.Dig));
            _brushes.Add(Brush.Type.Flatten, new Brush(this, Brush.Type.Flatten));
            _brushes.Add(Brush.Type.Drag, new Brush(this, Brush.Type.Drag));
            _brushes.Add(Brush.Type.Inflate, new Brush(this, Brush.Type.Inflate));
            _brushes.Add(Brush.Type.CADDrag, new Brush(this, Brush.Type.CADDrag));
        }

        #endregion // ! Init

        #region Brush Application

        public void StartStroke(Brush.Type brushType)
        {
            if (Editable)
            {
                _brushes.TryGetValue(brushType, out _currentBrush);
                if (_currentBrush != null)
                    _currentBrush.StartStroke();
            }
        }

        public void UpdateStroke(Brush.Type brushType, Vector3 origin, Vector3 direction, float rayLength, float radius, float strengthRatio)
        {
            if (Editable)
            {
                if (_currentBrush == null || brushType != _currentBrush.BrushType)
                {
                    StopStroke();
                    StartStroke(brushType);
                }
                if (_currentBrush != null)
                {
                    _currentBrush.UpdateStroke(gameObject.transform.localToWorldMatrix, origin, direction, rayLength, radius, strengthRatio);
                    UpdateEditableMesh();
                }
            }
            else
                StopStroke();
        }

        public void StopStroke()
        {
            if (_currentBrush != null)
            {
                _currentBrush.EndStroke();
                UpdateEditableMesh();
            }
        }

        #endregion // ! Brush application

        #region Mesh Updates

        private void UpdateEditableMesh()
        {
            if (MeshIsValid())
            {
                uint internalSubMeshCount = GetVertexCount();
                if (_shattered == null)
                {
                    if (internalSubMeshCount > 65000)
                    {
                        DivideMesh();
                        UpdateSubMeshes();
                    }
                    else
                    {
                        ReunifyMesh();
                        UpdateUnityMesh();
                    }
                }
                else if (_shattered == true)
                {
                    if (internalSubMeshCount < 40000)
                    {
                        ReunifyMesh();
                        UpdateUnityMesh();
                    }
                    else
                        UpdateSubMeshes();
                }
                else
                {
                    if (internalSubMeshCount > 65000)
                    {
                        DivideMesh();
                        UpdateSubMeshes();
                    }
                    else
                        UpdateUnityMesh();
                }
                UpdateBounds();
            }
        }


        private void DivideMesh()
        {
            Destroy(MeshFilter);
            MeshRenderer meshRenderer = gameObject.GetComponent<MeshRenderer>();
            if (meshRenderer != null)
            {
                _material = meshRenderer.material;
                Destroy(meshRenderer);
            }
            _shattered = true;
        }

        private void UpdateSubMeshes()
        {

            // Update sub meshes internal structure
            DLL.Mesh_UpdateSubMeshes(_internalMesh);

            // Update it for Unity
            uint nbMesh = GetSubMeshCount();

            // For every submesh in engine : Gets corresponding submesh ID from engine and checks if submesh with this id exists, updates submesh if yes, else creates new submesh.
            for (uint i = 0; i < nbMesh; ++i)
            {
                uint subMeshID = GetSubMeshID(i);
                SubMesh subMesh;
                if (_subMeshes.TryGetValue(subMeshID, out subMesh))
                    subMesh.Update();
                else
                    _subMeshes.Add(subMeshID, new SubMesh(gameObject, _internalMesh, i, _material));
            }
            // Remove submeshes that were deleted
            List<uint> removals = new List<uint>();
            foreach (uint id in _subMeshes.Keys)
            {
                if (!IsSubMeshExist(id))
                    removals.Add(id);
            }
            foreach (uint id in removals)
            {
                _subMeshes[id].Cleanup();
                _subMeshes.Remove(id);
            }
        }

        private void ReunifyMesh()
        {
            ClearSubMeshes();
            MeshRenderer meshRenderer = gameObject.GetComponent<MeshRenderer>();
            if (meshRenderer == null)
                meshRenderer = gameObject.AddComponent<MeshRenderer>();
            meshRenderer.material = _material;
            meshRenderer.receiveShadows = false;
            meshRenderer.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
            _shattered = false;
        }

        public void UpdateUnityMesh()
        {
            Mesh unityMesh = null;
            if (!_InPlayMode)
            {
                unityMesh = new Mesh();
                MeshFilter.sharedMesh = unityMesh;
            }
            else
            {
                if (MeshFilter.mesh != null)
                    MeshFilter.mesh = new Mesh();
                unityMesh = MeshFilter.mesh;
            }
            uint nbTris = DLL.Mesh_GetNbTriangles(_internalMesh);
            uint nbVtxs = DLL.Mesh_GetNbVertices(_internalMesh);
            int[] triangles = new int[nbTris];
            Vector3[] vertices = new Vector3[nbVtxs];
            Vector3[] normals = new Vector3[nbVtxs];
            GCHandle gcTriangles = GCHandle.Alloc(triangles, GCHandleType.Pinned);
            GCHandle gcVertices = GCHandle.Alloc(vertices, GCHandleType.Pinned);
            GCHandle gcNormals = GCHandle.Alloc(normals, GCHandleType.Pinned);
            DLL.Mesh_FillTriangles(_internalMesh, gcTriangles.AddrOfPinnedObject());
            DLL.Mesh_FillVertices(_internalMesh, gcVertices.AddrOfPinnedObject());
            DLL.Mesh_FillNormals(_internalMesh, gcNormals.AddrOfPinnedObject());
            gcTriangles.Free();
            gcVertices.Free();
            gcNormals.Free();
            unityMesh.Clear();
            unityMesh.vertices = vertices;
            unityMesh.normals = normals;
            unityMesh.triangles = triangles;
        }

        private void UpdateBounds()
        {
            if (MeshIsValid())
            {
                GCHandle gcMin = GCHandle.Alloc(_Bounds.min, GCHandleType.Pinned);
                GCHandle gcMax = GCHandle.Alloc(_Bounds.max, GCHandleType.Pinned);
                DLL.Mesh_GetBBox(_internalMesh, gcMin.AddrOfPinnedObject(), gcMax.AddrOfPinnedObject());
                _Bounds = new Bounds();
                _Bounds.min = Vector3.Scale((Vector3)gcMin.Target, transform.lossyScale);
                _Bounds.max = Vector3.Scale((Vector3)gcMax.Target, transform.lossyScale);
                gcMin.Free();
                gcMax.Free();
            }
        }

        #endregion // ! Mesh Updates

        #region Intersecton Tests

        public bool GetClosestIntersectionPoint(UnityEngine.Ray ray, ref UnityEngine.Vector3 intersectionPoint)
        {
            if (!MeshIsValid())
                return false;
            Matrix4x4 matrix = gameObject.transform.localToWorldMatrix;
            float[] rotAndScale3x3Matrix = { matrix.m00, matrix.m01, matrix.m02, matrix.m10, matrix.m11, matrix.m12, matrix.m20, matrix.m21, matrix.m22 };
            float[] position = { matrix.m03, matrix.m13, matrix.m23 };
            GCHandle gcRotAndScale3x3Matrix = GCHandle.Alloc(rotAndScale3x3Matrix, GCHandleType.Pinned);
            GCHandle gcPosition = GCHandle.Alloc(position, GCHandleType.Pinned);
            GCHandle gcRay = GCHandle.Alloc(ray, GCHandleType.Pinned);
            GCHandle gcIntersactionPoint = GCHandle.Alloc(intersectionPoint, GCHandleType.Pinned);
            bool intersect = DLL.Mesh_GetClosestIntersectionPoint(_internalMesh, gcRotAndScale3x3Matrix.AddrOfPinnedObject(), gcPosition.AddrOfPinnedObject(), gcRay.AddrOfPinnedObject(), gcIntersactionPoint.AddrOfPinnedObject(), IntPtr.Zero);
            intersectionPoint = (Vector3)gcIntersactionPoint.Target;
            gcIntersactionPoint.Free();
            gcRay.Free();
            gcRotAndScale3x3Matrix.Free();
            gcPosition.Free();
            return intersect;
        }

        public bool GetClosestIntersectionPoint(UnityEngine.Ray ray, ref UnityEngine.Vector3 intersectionPoint, ref UnityEngine.Vector3 intersectionNormal)
        {
            if (!MeshIsValid())
                return false;
            Matrix4x4 matrix = gameObject.transform.localToWorldMatrix;
            float[] rotAndScale3x3Matrix = { matrix.m00, matrix.m01, matrix.m02, matrix.m10, matrix.m11, matrix.m12, matrix.m20, matrix.m21, matrix.m22 };
            float[] position = { matrix.m03, matrix.m13, matrix.m23 };
            GCHandle gcRotAndScale3x3Matrix = GCHandle.Alloc(rotAndScale3x3Matrix, GCHandleType.Pinned);
            GCHandle gcPosition = GCHandle.Alloc(position, GCHandleType.Pinned);
            GCHandle gcRay = GCHandle.Alloc(ray, GCHandleType.Pinned);
            GCHandle gcIntersactionPoint = GCHandle.Alloc(intersectionPoint, GCHandleType.Pinned);
            GCHandle gcIntersactionNormal = GCHandle.Alloc(intersectionNormal, GCHandleType.Pinned);
            bool intersect = DLL.Mesh_GetClosestIntersectionPoint(_internalMesh, gcRotAndScale3x3Matrix.AddrOfPinnedObject(), gcPosition.AddrOfPinnedObject(), gcRay.AddrOfPinnedObject(), gcIntersactionPoint.AddrOfPinnedObject(), gcIntersactionNormal.AddrOfPinnedObject());
            intersectionPoint = (Vector3)gcIntersactionPoint.Target;
            intersectionNormal = (Vector3)gcIntersactionNormal.Target;
            gcIntersactionPoint.Free();
            gcIntersactionNormal.Free();
            gcRay.Free();
            gcRotAndScale3x3Matrix.Free();
            gcPosition.Free();
            return intersect;
        }

        #endregion // ! Intersections Tests

        #region Mesh Booleans

        private delegate bool BooleanOperationDelegate(IntPtr mesh, IntPtr otherMesh, IntPtr rotAndScale3x3Matrix, IntPtr position);

        private enum BooleanOperation { Subtract, Merge, Intersect }
        private Dictionary<BooleanOperation, BooleanOperationDelegate> booleanOperationsMap = new Dictionary<BooleanOperation, BooleanOperationDelegate>()
        {
            { BooleanOperation.Subtract,  DLL.CSGSubtract},
            { BooleanOperation.Merge,  DLL.CSGMerge},
            { BooleanOperation.Intersect,  DLL.CSGIntersect}
        };

        public bool Subtract(EditableMesh otherMesh)
        {
            return ApplyBooleanOperation(otherMesh, BooleanOperation.Subtract);
        }

        public bool Merge(EditableMesh otherMesh)
        {
            return ApplyBooleanOperation(otherMesh, BooleanOperation.Merge);
        }

        public bool Intersect(EditableMesh otherMesh)
        {
            return ApplyBooleanOperation(otherMesh, BooleanOperation.Intersect);
        }

        private bool ApplyBooleanOperation(EditableMesh otherMesh, BooleanOperation operation)
        {
            Matrix4x4 matrix = otherMesh.gameObject.transform.localToWorldMatrix;
            Matrix4x4 selfMatrix = gameObject.transform.localToWorldMatrix;
            matrix = selfMatrix.inverse * matrix;
            float[] rotAndScale3x3Matrix = { matrix.m00, matrix.m01, matrix.m02, matrix.m10, matrix.m11, matrix.m12, matrix.m20, matrix.m21, matrix.m22 };
            float[] position = { matrix.m03, matrix.m13, matrix.m23 };
            GCHandle gcRotAndScale3x3Matrix = GCHandle.Alloc(rotAndScale3x3Matrix, GCHandleType.Pinned);
            GCHandle gcPosition = GCHandle.Alloc(position, GCHandleType.Pinned);
            bool retValue = booleanOperationsMap[operation](GetInnerPtr(), otherMesh.GetInnerPtr(), gcRotAndScale3x3Matrix.AddrOfPinnedObject(), gcPosition.AddrOfPinnedObject());
            gcRotAndScale3x3Matrix.Free();
            gcPosition.Free();
            UpdateEditableMesh();
            return retValue;
        }

        #endregion // ! Editable Mesh

        private void ClearSubMeshes()
        {
            if (_subMeshes != null)
            {
                foreach (SubMesh subMesh in _subMeshes.Values)
                    subMesh.Cleanup();
                _subMeshes.Clear();
            }
        }

        public void Save(string filepath)
        {
            if (!MeshIsValid())
                return;
            DLL.MeshRecorder_Save(filepath, _internalMesh);
        }

        public uint GetVertexCount()
        {
            if (!MeshIsValid())
                return 0;
            return DLL.Mesh_GetNbVertices(_internalMesh);
        }

        public uint GetSubMeshCount()
        {
            if (!MeshIsValid())
                return 0;
            return DLL.Mesh_GetSubMeshCount(_internalMesh);
        }

        public bool CanUndo()
        {
            if (!MeshIsValid())
                return false;
            return DLL.Mesh_CanUndo(_internalMesh);
        }

        public void Undo(bool updateMesh = true)
        {
            if (!MeshIsValid())
                return;
            DLL.Mesh_Undo(_internalMesh);
            if(updateMesh)
                UpdateEditableMesh();
        }

        public bool CanRedo()
        {
            if (!MeshIsValid())
                return false;
            return DLL.Mesh_CanRedo(_internalMesh);
        }

        public void Redo(bool updateMesh = true)
        {
            if (!MeshIsValid())
                return;
            DLL.Mesh_Redo(_internalMesh);
            if (updateMesh)
                UpdateEditableMesh();
        }

        public uint GetID()
        {
            if (!MeshIsValid())
                return 0;
            return DLL.Mesh_GetID(_internalMesh);
        }

        public uint GetSubMeshID(uint subMeshIndex)
        {
            if (!MeshIsValid())
                return 0;
            return DLL.SubMesh_GetID(DLL.Mesh_GetSubMesh(_internalMesh, subMeshIndex));
        }

        public bool IsSubMeshExist(uint submeshID)
        {
            if (!MeshIsValid())
                return false;
            return DLL.Mesh_IsSubMeshExist(_internalMesh, submeshID);
        }
        
        private void DeleteInternalMesh()
        {
            if (!MeshIsValid())
                return;
            DLL.Mesh_Delete(_internalMesh);
            _internalMesh = IntPtr.Zero;
        }

        public IntPtr GetInnerPtr()
        {
            return _internalMesh;
        }

        private Material CreateDefaultMaterial()
        {
            Material mat = new Material(Shader.Find("TectridSDK/Cg per-pixel lighting"));
            mat.color = Color.white;
            return mat;
        }

        public bool MeshIsValid()
        {
            return _internalMesh != IntPtr.Zero;
        }

        private void OnDestroy()
        {
           SceneManager.Instance.UnregisterEditableMesh(this);
           DeleteInternalMesh();
        }
    }
}
