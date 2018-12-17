using UnityEngine;
using UnityEngine.UI;
using System;
using Tectrid;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using Tectrid.Primitives;

public class Main: MonoBehaviour
{
    public CameraOrbit _cameraScript;
    public GameObject _light;
    public Selectable _UndoButtonScript;
    public Selectable _RedoButtonScript;
    public Material _uiCursorMaterial;
    private GameObject _uiRingCursor;
    private Vector3 _intersectionPoint = Vector3.zero;
    private bool _sculpting = false;
    private Vector3 _oldMousePosition = Vector3.zero;
	static private bool _uiIsInUse = false;
	static public bool uiIsInUse { set { _uiIsInUse = value; } }
    private float _uiSculptingSize = 12.0f;
    private float _sculptingRadius = 0.12f;
    private float _modelRadius = 1.0f;
    private bool _GUIDoCantSaveModal = false;
    private bool _GUIDoInvalidMeshModal = false;

    [SerializeField]
    private GameObject _editableObject;

    private EditableMesh _fullMesh = null;

    public float uiSculptingSize
    {
        set
        {
            _uiSculptingSize = value;
            _sculptingRadius = _modelRadius * _uiSculptingSize / 100.0f;
        }
    }
    float _sculptingStrength = 0.5f;
    public float uiSculptingStrength
    {
        set
        {
            _sculptingStrength = value;
        }
    }
    private Brush.Type _brushType = Brush.Type.Draw;

    void Start()
	{
        Debug.Log("Main start");
        // Build mesh to sculpt
        _fullMesh = _editableObject.GetComponent<EditableMesh>();
        _cameraScript.m_target = _editableObject;
        // Build cursor
        CreateUIRingCursor();
        // Undo redo
        UpdateModelSizeRetargetCamera();
        UpdateUndoRedoButtonStates();
    }

	void Update()
	{
		bool hasPickPointToSculpt = false;
		Vector3 pickedScreenPoint = _oldMousePosition;
        Vector2 uiRingCursorScreenPos = Input.mousePosition;
        if (!_uiIsInUse && !_GUIDoCantSaveModal && !_GUIDoInvalidMeshModal)
        {
            if (Input.touchCount > 0/* && Input.GetTouch(0).phase == TouchPhase.Moved*/)
            {
                pickedScreenPoint = Input.GetTouch(0).position;
                uiRingCursorScreenPos = Input.GetTouch(0).position;
                if (_cameraScript.m_moveCamera == false)
                {
                    hasPickPointToSculpt = true;
                    _cameraScript.m_moveCamera = true;
                    _oldMousePosition = pickedScreenPoint;
                }
            }
            else if (Input.GetMouseButton(0))
            {
                pickedScreenPoint = Input.mousePosition;
                if (_cameraScript.m_moveCamera == false)
                {
                    hasPickPointToSculpt = true;
                    _cameraScript.m_moveCamera = true;
                    _oldMousePosition = pickedScreenPoint;
                }
            }
            else if (Input.GetMouseButton(1))
            {
                pickedScreenPoint = Input.mousePosition;
                if (_cameraScript.m_moveCamera == false)
                {
                    hasPickPointToSculpt = false;   // Only rotate with left mouse button
                    _cameraScript.m_moveCamera = true;
                    _oldMousePosition = pickedScreenPoint;
                }
            }
            else
                _cameraScript.m_moveCamera = false;
            if (hasPickPointToSculpt)
            {
                if (!_sculpting)
                {   // Test if we touch the mesh to activate sculpting
                    Ray ray = Camera.main.ScreenPointToRay(pickedScreenPoint);
                    bool intersect = _fullMesh.GetClosestIntersectionPoint(ray, ref _intersectionPoint);
                    if (intersect)
                    {   // Start sculpting brush stroke
                        Debug.Log("intersect");
                        _sculpting = true;
                        _fullMesh.StartStroke(_brushType);
                    }
                }
            }
            else
            {
                if (_sculpting)
                {   // Stop sculpting brush stroke
                    _sculpting = false;
                    _fullMesh.StopStroke();
                    UpdateModelSizeRetargetCamera();
                    UpdateUndoRedoButtonStates();
                }
            }
            if (_sculpting)
            {
                _cameraScript.m_moveCamera = false;
                // sculpting
                Ray ray = Camera.main.ScreenPointToRay(pickedScreenPoint);
                _fullMesh.UpdateStroke(_brushType, ray.origin, ray.direction, float.MaxValue, _sculptingRadius, _sculptingStrength);
            }
            else
            {
                _cameraScript.m_cameraDpl = (_oldMousePosition - pickedScreenPoint) / 10.0f;
                _oldMousePosition = pickedScreenPoint;
            }
        }
        else
            _cameraScript.m_moveCamera = false;
        // Update cursor
        UpdateCursor(uiRingCursorScreenPos);
        // Update light position regarding camera
        _light.transform.position = Camera.main.transform.position;
    }

	void OnDrawGizmos()
	{
        /*if(FullMesh != null)
		{
			foreach(Vector3 point in FullMesh.DEBUG_intersectionPoints)
				Gizmos.DrawSphere(point, 0.1f);
            foreach(SubMesh subMesh in _subMeshes)
                Gizmos.DrawWireCube(subMesh.bbox.center, subMesh.bbox.size);
		}*/
    }

    private void CreateUIRingCursor()
    {
        // Create wireframe ring (diameter 1.0)
        uint nbSegments = 100;
        float stepAngle = (2.0f * Mathf.PI) / (float) nbSegments;
        Vector3[] circleVertices = new Vector3[nbSegments + 1];
        float curAngle = 0.0f;
        int[] circleIndices = new int[nbSegments + 1];
        for(int i = 0; i <= nbSegments; ++i, curAngle += stepAngle)
        {
            circleVertices[i].x = Mathf.Cos(curAngle);
            circleVertices[i].y = 0.0f;
            circleVertices[i].z = Mathf.Sin(curAngle);
            circleIndices[i] = i;
        }
        // Create unity Mesh
        UnityEngine.Mesh uiRingCursorMesh = new UnityEngine.Mesh();
        uiRingCursorMesh.name = "mesh";
        uiRingCursorMesh.vertices = circleVertices;
        uiRingCursorMesh.SetIndices(circleIndices, MeshTopology.LineStrip, 0);
        // Register it to Unity using a sub game entity to be able to freely control its transform
        _uiRingCursor = new GameObject();
        _uiRingCursor.name = "RingCursor";
        _uiRingCursor.transform.SetParent(transform);
        MeshRenderer mr = _uiRingCursor.AddComponent<MeshRenderer>();
        mr.material = _uiCursorMaterial;
        //mr.material.renderQueue = (int) UnityEngine.Rendering.RenderQueue.Overlay;
        MeshFilter mf = _uiRingCursor.AddComponent<MeshFilter>();
        mf.mesh = uiRingCursorMesh;
        _uiRingCursor.transform.localScale = new Vector3(0.0f, 0.0f, 0.0f);
    }

    private void UpdateCursor(Vector2 screenPos)
    {
        // Update cursor
        Ray ray = Camera.main.ScreenPointToRay(screenPos);
        Vector3 cursorcIntersectPoint = Vector3.zero;
        Vector3 cursorcIntersectNormal = Vector3.zero;
        bool intersect = _fullMesh.GetClosestIntersectionPoint(ray, ref cursorcIntersectPoint, ref cursorcIntersectNormal);
        if(intersect)
        {
            Vector3 at = cursorcIntersectNormal;
            Vector3 up = Vector3.up; // 0, 1, 0
            if(Math.Abs(Vector3.Dot(up, at)) > 0.95)
                up = Vector3.right; // 1, 0, 0
            Vector3 left = Vector3.Cross(up, at);
            left.Normalize();
            up = Vector3.Cross(left, at);
            up.Normalize();
            _uiRingCursor.transform.rotation = Quaternion.LookRotation(left, at);
            _uiRingCursor.transform.position = cursorcIntersectPoint;
            _uiRingCursor.transform.localScale = new Vector3(_sculptingRadius, _sculptingRadius, _sculptingRadius);
        }
        else if(Camera.main != null)
        {
            Vector3 uiRingPos = ray.origin + (ray.direction * ray.origin.magnitude);
            _uiRingCursor.transform.localPosition = uiRingPos;
            _uiRingCursor.transform.localScale = new Vector3(_sculptingRadius, _sculptingRadius, _sculptingRadius);
            _uiRingCursor.transform.localRotation = Camera.main.transform.localRotation * Quaternion.Euler(90.0f, 0.0f, 0.0f);
        }
    }

    public void SetDrawBrush()
    {
        _brushType = Tectrid.Brush.Type.Draw;
    }

    public void SetInflateBrush()
    {
        _brushType = Tectrid.Brush.Type.Inflate;
    }

    public void SetFlattenBrush()
    {
        _brushType = Tectrid.Brush.Type.Flatten;
    }

    public void SetDragBrush()
    {
        _brushType = Tectrid.Brush.Type.Drag;
    }

    public void SetDigBrush()
    {
        _brushType = Tectrid.Brush.Type.Dig;
    }

    public void SetCADDragBrush()
    {
        _brushType = Tectrid.Brush.Type.CADDrag;
    }

    public void SwitchFullScreen()
	{
        UnityEngine.Screen.fullScreen = !UnityEngine.Screen.fullScreen;
	}

    public void LoadFile()
    {
        _fullMesh.Build((string) null);
        UpdateModelSizeRetargetCamera();
    }

    void CantSaveModalCallBack(int windowID)
    {
        if(GUI.Button(new Rect(440.0f, 130.0f, 120, 40), "OK"))
            _GUIDoCantSaveModal = false;
    }

    void InvalidMeshModalCallBack(int windowID)
    {
        if(GUI.Button(new Rect(420.0f, 155.0f, 120, 40), "OK"))
            _GUIDoInvalidMeshModal = false;
    }

    public void Undo()
    {
        _fullMesh.Undo();
        UpdateModelSizeRetargetCamera();
        UpdateUndoRedoButtonStates();
    }

    public void Redo()
    {
        _fullMesh.Redo();
        UpdateModelSizeRetargetCamera();
        UpdateUndoRedoButtonStates();
    }

    void OnGUI()
    {
        if(_GUIDoCantSaveModal)
        {
            Rect windowRect = new Rect((Screen.width / 2.0f) - 500.0f, (Screen.height / 2.0f) - 90.0f, 1000.0f, 180.0f);
            GUIStyle windowStyle = new GUIStyle(GUI.skin.button);
            windowStyle.fontSize = 25;
            windowStyle.alignment = TextAnchor.UpperCenter;
            GUI.ModalWindow(0, windowRect, CantSaveModalCallBack, "Saving to file is turned off\n\nThis is a demo version, that's why saving to file is turned off.\nIf you're interested with what you've seen, please visit www.tectrid.com contact page.", windowStyle);
        }
        if(_GUIDoInvalidMeshModal)
        {
            Rect windowRect = new Rect((Screen.width / 2.0f) - 450.0f, (Screen.height / 2.0f) - 105.0f, 900.0f, 210.0f);
            GUIStyle windowStyle = new GUIStyle(GUI.skin.button);
            windowStyle.fontSize = 25;
            windowStyle.alignment = TextAnchor.UpperCenter;
            GUI.ModalWindow(0, windowRect, InvalidMeshModalCallBack, "Invalid mesh input\n\nSculpting engine only supports manifold closed meshes for the moment.\n(A manifold mesh is a mesh that could be represented in \"real life\")\nThe input mesh doesn't fit those constraints and therefore has been rejected.", windowStyle);
        }
    }

    public void SaveFile()
    {
        _GUIDoCantSaveModal = true;
        //Tectrid.MeshRecorder_Save(null, FullMesh);  // Passing null will trigger file save dialog in the plugin
    }

    public void LoadBox()
    {
        _fullMesh.Build(new Box(10, 10, 10));
        UpdateModelSizeRetargetCamera();
    }

    public void LoadSphere()
    {
        _fullMesh.Build(new Sphere(10));
        UpdateModelSizeRetargetCamera();
    }

    public void SetMirrorMode(bool value)
    {
        Tectrid.Parameters.SetMirrorMode(value);
    }

    private void UpdateModelSizeRetargetCamera()
    {
        // Compute mesh bbox from sub meshes
        Bounds meshBbox = _fullMesh.Bounds;
        //float M_SQRT2 = 1.41421356237309504880f;
        //_modelRadius = meshBbox.size.magnitude * (0.5f / M_SQRT2);
        _modelRadius = Math.Max(Math.Max(meshBbox.extents.x, meshBbox.extents.y), meshBbox.extents.z);
        // Force camera retarget
        _cameraScript.SetTargetBounds(meshBbox);
        _cameraScript.ForceRetarget();
        _sculptingRadius = _modelRadius * _uiSculptingSize / 100.0f;
    }

    void UpdateUndoRedoButtonStates()
    {
        if(_UndoButtonScript != null)
            _UndoButtonScript.interactable = _fullMesh.CanUndo();
        if(_RedoButtonScript != null)
            _RedoButtonScript.interactable = _fullMesh.CanRedo();
    }
}