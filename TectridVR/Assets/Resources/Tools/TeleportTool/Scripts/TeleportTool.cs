using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(TargetingTool))]
[RequireComponent(typeof(TeleportToolInputHandler))]
public class TeleportTool : VRTool
{
    public TeleportToolInputHandler _inputManager;
    
    private GameObject _uiRingCursor = null;
    private TargetingTool _targetingTool;
    [SerializeField]
    private Material _uiCursorMaterial;
    [SerializeField]
    private float _ringRadius;

    [SerializeField]
    private GameObject player;

    // Use this for initialization
    void Start ()
    {
        _inputManager = transform.GetComponent<TeleportToolInputHandler>();
        _inputManager.TeleportInputPressed += Teleport;
        _targetingTool = transform.GetComponent<TargetingTool>();
        CreateUIRingCursor();
    }

    void Update ()
    {
        UpdateCursor();
	}

    private void Teleport()
    {
        if (_uiRingCursor.activeSelf)
        {
            player.transform.position = new Vector3(_uiRingCursor.transform.position.x, player.transform.position.y, _uiRingCursor.transform.position.z);
        }
    }


    #region Teleport Ring
    private void CreateUIRingCursor()
    {
        // Create wireframe ring (diameter 1.0)
        uint nbSegments = 150;
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
        _uiRingCursor = new GameObject();
        _uiRingCursor.name = "TeleportRingCursor";
        _uiRingCursor.transform.SetParent(transform);
        MeshRenderer mr = _uiRingCursor.AddComponent<MeshRenderer>();
        mr.material = _uiCursorMaterial;
        //mr.material.renderQueue = (int) UnityEngine.Rendering.RenderQueue.Overlay;
        MeshFilter mf = _uiRingCursor.AddComponent<MeshFilter>();
        mf.mesh = uiRingCursorMesh;
        _uiRingCursor.transform.localScale = new Vector3(0.0f, 0.0f, 0.0f);
    }

    private void UpdateCursor()
    {
        // Update cursor
        Ray ray = new Ray(transform.position, transform.forward);
        RaycastHit hitInfos;
        if ((Physics.Raycast(ray, out hitInfos)))
        {
            if (hitInfos.collider.gameObject.tag == "Floor")
            {
                _uiRingCursor.SetActive(true);
                Vector3 at = hitInfos.normal;
                Vector3 up = Vector3.up; // 0, 1, 0
                if (Math.Abs(Vector3.Dot(up, at)) > 0.95)
                    up = Vector3.right; // 1, 0, 0
                Vector3 left = Vector3.Cross(up, at);
                left.Normalize();
                up = Vector3.Cross(left, at);
                up.Normalize();
                _uiRingCursor.transform.rotation = Quaternion.LookRotation(left, at);
                _uiRingCursor.transform.position = hitInfos.point;
                _uiRingCursor.transform.localScale = new Vector3(_ringRadius, _ringRadius, _ringRadius);
            }
            else
                _uiRingCursor.SetActive(false);
        }
        else
            _uiRingCursor.SetActive(false);
    }
#endregion // ! Teleport Ring
}
