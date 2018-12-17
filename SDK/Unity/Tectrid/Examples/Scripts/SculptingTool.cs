using System;
using System.Collections;
using System.Collections.Generic;
using Tectrid;
using UnityEngine;

namespace Tectrid.Examples
{
    public class SculptingTool : MonoBehaviour
    {
        public EditableMesh Target;

        private Brush.Type _brushType;

        private float _modelRadius;
        private float _uiBrushRadius = 12.0f;
        private float _brushRadius = 1;
        private float _brushIntensity = 1;

        private Vector3 _previousMousePos;
        private bool _sculpting;
        private bool _rotating;
        // Use this for initialization
        void Start()
        {
            CreateUIRingCursor();
        }

 
        // Update is called once per frame
        void Update()
        {
            /* if (Input.GetMouseButtonDown(0) || Input.GetMouseButtonDown(1))
             {
                 Vector3 mousePos = Input.mousePosition;
                 Ray ray = Camera.main.ScreenPointToRay(mousePos);
                 EditableMesh meshHit = SceneManager.Instance.GetClosestMesh(ray);
             } */
            Vector3 mousePos = Input.mousePosition;
            Ray ray = Camera.main.ScreenPointToRay(mousePos);
            Debug.DrawRay(ray.origin, ray.direction, Color.red);
            EditableMesh meshHit = SceneManager.Instance.GetClosestMesh(ray);
            if (!_sculpting && !_rotating)
            {
                Target = meshHit;
                // Compute mesh bbox from sub meshes
                if (Target != null)
                {
                    Bounds meshBbox = Target.Bounds;
                    _modelRadius = Math.Max(Math.Max(meshBbox.extents.x, meshBbox.extents.y), meshBbox.extents.z);
                    _brushRadius = _modelRadius * _uiBrushRadius / 100.0f;
                }
            }

            if (Target != null)
            {
                if (Input.GetAxisRaw("Mouse ScrollWheel") < 0)
                    Undo();
                else if (Input.GetAxisRaw("Mouse ScrollWheel") > 0)
                    Redo();
                ApplySculptInput(ray);
                ApplyRotateInput(ray, mousePos);
                UpdateCursor(mousePos);
            }
            _previousMousePos = mousePos;
        }

        void ApplySculptInput(Ray ray)
        {
            if (Input.GetMouseButtonDown(0))
            {
                _sculpting = true;
                Target.StartStroke(_brushType);
            }
            if (Input.GetMouseButton(0))
            {
                if (!_sculpting)
                {
                    Target.StartStroke(_brushType);
                    _sculpting = true;
                }
                Target.UpdateStroke(_brushType, ray.origin, ray.direction, float.MaxValue, _brushRadius, _brushIntensity);
            }
            if (Input.GetMouseButtonUp(0))
            {
                /* if (!Input.GetMouseButton(1))
                     Target = null; */
                Target.StopStroke();
                _sculpting = false;
            }
        }

        void ApplyRotateInput(Ray ray, Vector3 mousePos)
        {
            if (Input.GetMouseButton(1))
            {
                Target.transform.Rotate(new Vector3(mousePos.y - _previousMousePos.y, _previousMousePos.x - mousePos.x), Space.World);
                _rotating = true;
            }
            if (Input.GetMouseButtonUp(1))
            {
                /* if (!Input.GetMouseButton(0))
                     Target = null; */
                _rotating = false;
            }
        }

        private void CreateUIRingCursor()
        {
            // Create wireframe ring (diameter 1.0)
            uint nbSegments = 100;
            float stepAngle = (2.0f * Mathf.PI) / (float)nbSegments;
            Vector3[] circleVertices = new Vector3[nbSegments + 1];
            float curAngle = 0.0f;
            int[] circleIndices = new int[nbSegments + 1];
            for (int i = 0; i <= nbSegments; ++i, curAngle += stepAngle)
            {
                circleVertices[i].x = Mathf.Cos(curAngle);
                circleVertices[i].y = 0.0f;
                circleVertices[i].z = Mathf.Sin(curAngle);
                circleIndices[i] = i;
            }
            // Create unity Mesh
            Mesh uiRingCursorMesh = new Mesh();
            uiRingCursorMesh.name = "mesh";
            uiRingCursorMesh.vertices = circleVertices;
            uiRingCursorMesh.SetIndices(circleIndices, MeshTopology.LineStrip, 0);
            // Register it to Unity using a sub game entity to be able to freely control its transform
            //mr.material.renderQueue = (int) UnityEngine.Rendering.RenderQueue.Overlay;
            MeshFilter mf = gameObject.GetComponent<MeshFilter>();
            if (mf == null)
                mf = gameObject.AddComponent<MeshFilter>();
            mf.mesh = uiRingCursorMesh;
            transform.localScale = new Vector3(0.0f, 0.0f, 0.0f);
        }


        private void UpdateCursor(Vector2 screenPos)
        {
            // Update cursor
            Ray ray = Camera.main.ScreenPointToRay(screenPos);
            Vector3 cursorcIntersectPoint = Vector3.zero;
            Vector3 cursorcIntersectNormal = Vector3.zero;
            bool intersect = Target.GetClosestIntersectionPoint(ray, ref cursorcIntersectPoint, ref cursorcIntersectNormal);
            if (intersect)
            {
                Vector3 at = cursorcIntersectNormal;
                Vector3 up = Vector3.up; // 0, 1, 0
                if (Math.Abs(Vector3.Dot(up, at)) > 0.95)
                    up = Vector3.right; // 1, 0, 0
                Vector3 left = Vector3.Cross(up, at);
                left.Normalize();
                up = Vector3.Cross(left, at);
                up.Normalize();
                transform.rotation = Quaternion.LookRotation(left, at);
                transform.position = cursorcIntersectPoint;
                transform.localScale = new Vector3(_brushRadius, _brushRadius, _brushRadius);
            }
            else if (Camera.main != null)
            {
                Vector3 uiRingPos = ray.origin + (ray.direction * ray.origin.magnitude);
                transform.localPosition = uiRingPos;
                transform.localScale = new Vector3(_brushRadius, _brushRadius, _brushRadius);
                transform.localRotation = Camera.main.transform.localRotation * Quaternion.Euler(90.0f, 0.0f, 0.0f);
            }
        }

        public void SetBrushToDraw()
        {
            _brushType = Brush.Type.Draw;
        }

        public void SetBrushToDig()
        {
            _brushType = Brush.Type.Dig;
        }

        public void SetBrushToDrag()
        {
            _brushType = Brush.Type.Drag;
        }

        public void SetBrushToFlatten()
        {
            _brushType = Brush.Type.Flatten;
        }

        public void SetBrushToCADDrag()
        {
            _brushType = Brush.Type.CADDrag;
        }

        public void ChangeBrushIntensity(float brushIntensity)
        {
            _brushIntensity = brushIntensity;
        }

        public void ChangeBrushRadius(float uiBrushRadius)
        {
            _uiBrushRadius = uiBrushRadius;
            _brushRadius = _modelRadius * _uiBrushRadius / 100.0f;
        }

        public void Undo()
        {
            Target.Undo();
        }

        public void Redo()
        {
            Target.Redo();
        }
    }
}

