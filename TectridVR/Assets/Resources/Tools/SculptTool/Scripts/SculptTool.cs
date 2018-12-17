using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Tectrid;
using System;

[RequireComponent(typeof(SteamVR_TrackedController))]
[RequireComponent(typeof(TargetingTool))]
[RequireComponent(typeof(SculptToolInputHandler))]
public class SculptTool : VRTool
{
    private bool _Erasing;    
    public bool Erasing
    {
        get { return _Erasing; }
        set
        {
            _Erasing = value;
            if (_uiRingCursor != null)
            {
                if (_Erasing)
                    DrawEraseCursor();
                else
                    DrawSculptCursor();
            }
        }
    }

    [SerializeField] private Material _uiCursorSculptMaterial;
    [SerializeField] private Material _uiCursorEraseMaterial;
    [SerializeField] private float _defaultRingCursorOffset = 10;
    [SerializeField] private float _brushRadiusChangesPerInput = 1f;
    [SerializeField] private float _brushStrengthChangesPerInput = 0.5f;
    [SerializeField] private BrushParametersUI _brushParameterUI;

    private SculptToolInputHandler _inputManager;
    private EditableMesh _target;
    private bool _sculpting;
    private TargetingTool _targetingTool;
    private Brush.Type _brushType;
    private GameObject _uiRingCursor = null;
    private float _modelRadius = 1;
    private float _brushStrength = 10;
    private float _uiBrushRadius;
    private float _brushRadius;
    public float BrushRadius
    {
        get { return _uiBrushRadius; }
        set
        {
            _uiBrushRadius = value;
            _brushRadius = _modelRadius * value / 100.0f;
        }
    }

    void Start ()
    {
        _target = null;
        BrushRadius = 10;
        _inputManager = transform.GetComponent<SculptToolInputHandler>();
        _inputManager.StrokeStartInputPressed += StartStroke;
        _inputManager.StrokeStopInputPressed += StopStroke;
        _inputManager.StrokeUpdateInputPressed += UpdateStroke;
        _inputManager.BrushRadiusChangeInputPressed += UpdateBrushRadius;
        _inputManager.BrushStrengthChangeInputPressed += UpdateBrushStrength;
        _inputManager.UndoInputPressed += Undo;
        _inputManager.RedoInputPressed += Redo;
        _targetingTool = transform.GetComponent<TargetingTool>();
        CreateCursor();
        _brushParameterUI.UpdateBrushRadius(BrushRadius);
        _brushParameterUI.UpdateBrushStrength(_brushStrength);
        Erasing = false;
    }

    void Update ()
    {
        EditableMesh currentTarget = _targetingTool.Target != null ? _targetingTool.Target.GetComponent<EditableMesh>() : null;
        if (currentTarget != _target && !(_sculpting && _brushType == Brush.Type.Drag))
        {
            if (_sculpting && currentTarget != null && _target != null)
            {
                _target.StopStroke();
                _target = currentTarget;
                StartStroke();
            }
            else if(currentTarget != null)
                _target = currentTarget;
        }
        if (_target != null)
            ComputeModelRadius();
        if (!_sculpting || _brushType != Brush.Type.Drag)
            UpdateCursor();
	}

    #region Input Events Handler

    void StartStroke()
    {
        if (_target != null && enabled)
        {
            if (Erasing)
            {
                Destroy(_target.gameObject);
                _target = null;
            }
            else
            {
                _target.StartStroke(_brushType);
                _sculpting = true;
            }        
        }
    }
    
    void UpdateStroke()
    {
        if (_target != null && _sculpting && enabled)
        {
            _target.UpdateStroke(_brushType, transform.position, transform.forward, float.MaxValue, _brushRadius, _brushStrength * 0.5f / 15.0f);
        }
    }

    void StopStroke()
    {
        if (_target != null && enabled)
        {
            _target.StopStroke();
            SceneManager.Instance.AddToUndoRedoList(_target);
        }
        _sculpting = false;
    }

    void UpdateBrushRadius()
    {
        if (enabled)
        {
            BrushRadius += _inputManager.Inputs.BrushRadiusChange * _brushRadiusChangesPerInput;
            BrushRadius = Mathf.Clamp(BrushRadius, 1, 30);
            _brushParameterUI.UpdateBrushRadius(BrushRadius);
        }
    }

    void UpdateBrushStrength()
    {
        if (enabled)
        {
            _brushStrength += _inputManager.Inputs.BrushStrengthChange * _brushStrengthChangesPerInput;
            _brushStrength = Mathf.Clamp(_brushStrength, 0.5f, 15);
            _brushParameterUI.UpdateBrushStrength(_brushStrength);
        }
    }

    #endregion // !  Input Events Handler

    #region Preview Ring

    private void CreateCursor()
    {
        _uiRingCursor = new GameObject();
        _uiRingCursor.name = "RingCursor";
        _uiRingCursor.transform.SetParent(transform);
        _uiRingCursor.AddComponent<MeshRenderer>();
        _uiRingCursor.AddComponent<MeshFilter>();
    }

    private void DrawEraseCursor()
    {
        uint nbSegments = 200;
        float stepAngle = (2.0f * Mathf.PI) / (float)nbSegments;
        Vector3[] circleVertices = new Vector3[nbSegments + 5 + 1];
        float curAngle = Mathf.PI / 4;
        int[] circleIndices = new int[nbSegments + 5 + 1];
        int i = 0;
        while (i <= nbSegments)
        {
            circleVertices[i].x = Mathf.Cos(curAngle);
            circleVertices[i].y = 0.0f;
            circleVertices[i].z = Mathf.Sin(curAngle);
            circleIndices[i] = i;
            i++;
            curAngle += stepAngle;
        }
        circleVertices[i].x = Mathf.Cos(Mathf.PI / 4);
        circleVertices[i].y = 0.0f;
        circleVertices[i].z = Mathf.Sin(Mathf.PI / 4);
        circleIndices[i] = i;
        i++;
        circleVertices[i].x = Mathf.Cos(Mathf.PI + Mathf.PI / 4);
        circleVertices[i].y = 0.0f;
        circleVertices[i].z = Mathf.Sin(Mathf.PI + Mathf.PI / 4);
        circleIndices[i] = i;
        i++;
        circleVertices[i].x = 0;
        circleVertices[i].y = 0.0f;
        circleVertices[i].z = 0;
        circleIndices[i] = i;
        i++; 
        circleVertices[i].x = Mathf.Cos(Mathf.PI - Mathf.PI / 4);
        circleVertices[i].y = 0.0f;
        circleVertices[i].z = Mathf.Sin(Mathf.PI - Mathf.PI / 4);
        circleIndices[i] = i;
        i++;
        circleVertices[i].x = Mathf.Cos(Mathf.PI + Mathf.PI - Mathf.PI / 4);
        circleVertices[i].y = 0.0f;
        circleVertices[i].z = Mathf.Sin(Mathf.PI + Mathf.PI - Mathf.PI / 4);
        circleIndices[i] = i;
        // Create unity Mesh
        Mesh uiRingCursorMesh = new UnityEngine.Mesh();
        uiRingCursorMesh.name = "mesh";
        uiRingCursorMesh.vertices = circleVertices;
        uiRingCursorMesh.SetIndices(circleIndices, MeshTopology.LineStrip, 0);
        MeshRenderer mr = _uiRingCursor.GetComponent<MeshRenderer>();
        mr.material = _uiCursorEraseMaterial;
        MeshFilter mf = _uiRingCursor.GetComponent<MeshFilter>();

        mf.mesh = uiRingCursorMesh;
        _uiRingCursor.transform.localScale = new Vector3(0.0f, 0.0f, 0.0f);
    }

    private void DrawSculptCursor()
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
        Mesh uiRingCursorMesh = new UnityEngine.Mesh();
        uiRingCursorMesh.name = "mesh";
        uiRingCursorMesh.vertices = circleVertices;
        uiRingCursorMesh.SetIndices(circleIndices, MeshTopology.LineStrip, 0);
        // Register it to Unity using a sub game entity to be able to freely control its transform
        MeshRenderer mr = _uiRingCursor.GetComponent<MeshRenderer>();
        mr.material = _uiCursorSculptMaterial;
        //mr.material.renderQueue = (int) UnityEngine.Rendering.RenderQueue.Overlay;
        MeshFilter mf = _uiRingCursor.GetComponent<MeshFilter>();
        mf.mesh = uiRingCursorMesh;
        _uiRingCursor.transform.localScale = new Vector3(0.0f, 0.0f, 0.0f);
    }

    private void UpdateCursor()
    {
        // Update cursor
        Ray ray = new Ray(transform.position, transform.forward);
        Vector3 cursorcIntersectPoint = Vector3.zero;
        Vector3 cursorcIntersectNormal = Vector3.zero;
        bool intersect = false;
        if (_target != null)
            intersect = _target.GetClosestIntersectionPoint(ray, ref cursorcIntersectPoint, ref cursorcIntersectNormal);
        if (intersect)
        {
            _uiRingCursor.GetComponent<MeshRenderer>().enabled = true;
            Vector3 at = cursorcIntersectNormal;
            Vector3 up = Vector3.up; // 0, 1, 0
            if (Math.Abs(Vector3.Dot(up, at)) > 0.95)
                up = Vector3.right; // 1, 0, 0
            Vector3 left = Vector3.Cross(up, at);
            left.Normalize();
            up = Vector3.Cross(left, at);
            up.Normalize();
            _uiRingCursor.transform.rotation = Quaternion.LookRotation(left, at);
            _uiRingCursor.transform.position = cursorcIntersectPoint + cursorcIntersectNormal.normalized * 0.01f;
            _uiRingCursor.transform.localScale = new Vector3(_brushRadius, _brushRadius, _brushRadius);
        }
        else
            _uiRingCursor.GetComponent<MeshRenderer>().enabled = false;
    }

    #endregion // ! Preview Ring

    public void SelectBrush(Brush.Type brushType)
    {
        _brushType = brushType;
    }

    public void OnEnable()
    {
        if (_uiRingCursor != null)
            _uiRingCursor.SetActive(true);
    }

    public void OnDisable()
    {
        if (_uiRingCursor != null)
            _uiRingCursor.SetActive(false);
        _sculpting = false;
    }

    private void ComputeModelRadius()
    {
        Bounds meshBounds = _target.Bounds;
        _modelRadius = Math.Max(Math.Max(meshBounds.extents.x, meshBounds.extents.y), meshBounds.extents.z);
        BrushRadius = _uiBrushRadius;
    }

    private void Undo()
    {
        SceneManager.Instance.Undo();
    }

    private void Redo()
    {
        SceneManager.Instance.Redo();
    }
}
