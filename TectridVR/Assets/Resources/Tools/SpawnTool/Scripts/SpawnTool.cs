using System;
using System.Collections;
using System.Collections.Generic;
using Tectrid;
using Tectrid.Primitives;
using UnityEngine;

[RequireComponent(typeof(SteamVR_TrackedController))]
[RequireComponent(typeof(TargetingTool))]
public class SpawnTool : VRTool
{
    private TargetingTool _targetingTool;
    private GameObject _previewObject = null;
    private EditableMesh _previewObjectMesh;
    private SpawnToolInputHandler _inputManager;

    [SerializeField] private float _distanceChangePerInputs = 0.1f;
    [SerializeField] private float _percentScaleChangePerInputs = 0.1f;

    private float _previewObjectScale = 1;
    private float _previewObjectOffset = 3;

    private bool _Copying = false;
    public bool Copying
    {
        get { return _Copying; }
        set
        {
            if (_Copying != value)
            {
                _Copying = value;
                if (_previewObject != null)
                {
                    _previewObject.SetActive(!_Copying);
                }
                _copyTarget = null;
            }
        }
    }
    private EditableMesh _copyTarget;

    void OnEnable()
    {
        if (_previewObject == null)
        {
            _previewObject = new GameObject();
            _previewObject.transform.SetParent(transform);
            _previewObject.tag = "Spawning";
            _previewObjectMesh = _previewObject.AddComponent<EditableMesh>();
            BuildObjectMesh(Primitive.Types.Sphere);
            _previewObject.transform.position = transform.position + transform.forward * _previewObjectOffset;
        }
    }

    private void Start()
    {
        _targetingTool = transform.GetComponent<TargetingTool>();
        _inputManager = transform.GetComponent<SpawnToolInputHandler>();
        _inputManager.SpawnButtonPressed += SpawnObject;
        _inputManager.ObjectSizeChangeInputPressed += ChangeObjectSize;
        _inputManager.ObjectDistanceChangeInputPressed += ChangeObjectDistance;
    }

    private void Update()
    {
        if (Copying)
        {
            if (_targetingTool.Target != null && _targetingTool.Target.GetComponent<EditableMesh>() != null)
            {
                _copyTarget = _targetingTool.Target.GetComponent<EditableMesh>();
            }
            else
                _copyTarget = null;
        }
        else
        {
            if (_targetingTool.Target != null && Vector3.Distance(transform.position, _targetingTool.Target.transform.position) < 4 && _targetingTool.Target.GetComponent<EditableMesh>() == null)
            {
                if (_targetingTool.Target != _previewObject && _previewObject.activeSelf)
                    _previewObject.SetActive(false);
            }
            else
            {
                if (!_previewObject.activeSelf)
                    _previewObject.SetActive(true);
            }
            _previewObject.transform.position = transform.position + transform.forward.normalized * _previewObjectOffset + transform.forward.normalized * ComputeModelRadius();
        }
    }

    private void OnDisable()
    {
        _previewObject.SetActive(false);
    }

    public void SpawnObject()
    {
        if (Copying)
        {
            if (_copyTarget != null)
            {
                _previewObjectMesh.Build(_copyTarget.GetInnerPtr());
                _previewObjectScale = 1;
                _previewObject.transform.localScale = _copyTarget.transform.localScale;
                _previewObject.transform.localRotation = _copyTarget.transform.rotation;
                Copying = false;
            }
        }
        else
        {
            if (_previewObject.activeSelf)
            {
                IntPtr _prevMesh = _previewObjectMesh.GetInnerPtr();
                _previewObject.transform.SetParent(null);
                _previewObject.tag = "Untagged";
                _previewObject = new GameObject();
                _previewObject.transform.SetParent(transform);
                _previewObject.transform.position += transform.forward * 3;
                _previewObjectMesh = _previewObject.AddComponent<EditableMesh>();
                _previewObject.tag = "Spawning";
                _previewObject.transform.localScale = new Vector3(_previewObjectScale, _previewObjectScale, _previewObjectScale);
                _previewObjectMesh.Build(_prevMesh);
            }
        }

    }

    private float ComputeModelRadius()
    {
        Bounds meshBounds = _previewObjectMesh.Bounds;
        return (Math.Max(Math.Max(meshBounds.extents.x, meshBounds.extents.y), meshBounds.extents.z));
    }


    private void ChangeObjectDistance()
    {
        _previewObjectOffset += _distanceChangePerInputs * _inputManager.Inputs.ObjectDistanceChange;
        _previewObjectOffset = Mathf.Clamp(_previewObjectOffset, 1, 10);
    }

    private void ChangeObjectSize()
    {
        _previewObjectScale *= 1 + _inputManager.Inputs.ObjectSizeChange * (_percentScaleChangePerInputs / 100);
        _previewObjectScale = Mathf.Clamp(_previewObjectScale, 0.1f, 5);
        _previewObject.transform.localScale = new Vector3(_previewObjectScale, _previewObjectScale, _previewObjectScale);
    }

    #region Mesh Building

    public void BuildObjectMesh(Primitive.Types primitiveType)
    {
        Copying = false;
        _previewObjectScale = 10f;
        switch (primitiveType)
        {
            case Primitive.Types.Box:
                _previewObjectMesh.Build(new Box(_previewObjectScale, _previewObjectScale, _previewObjectScale));
                break;
            case Primitive.Types.Sphere:
            default:
                _previewObjectMesh.Build(new Sphere(_previewObjectScale));
                break;
        }
        _previewObjectScale = 0.1f;
        _previewObject.transform.localScale = new Vector3(_previewObjectScale, _previewObjectScale, _previewObjectScale);
    }

    public void BuildObjectMesh(string file)
    {
        Copying = false;
        _previewObjectMesh.Build(file);
        _previewObjectScale = 1;
        _previewObject.transform.localScale = new Vector3(_previewObjectScale, _previewObjectScale, _previewObjectScale);

    }

    public void BuildObjectMesh(Mesh mesh)
    {
        Copying = false;
        _previewObjectMesh.Build(mesh);
        _previewObjectScale = 1;
        _previewObject.transform.localScale = new Vector3(_previewObjectScale, _previewObjectScale, _previewObjectScale);

    }

    #endregion // ! Mesh Building


}
