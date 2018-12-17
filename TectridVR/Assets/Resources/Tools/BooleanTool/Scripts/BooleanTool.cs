using System.Collections;
using System.Collections.Generic;
using Tectrid;
using UnityEngine;

[RequireComponent(typeof(BooleanToolInputHandler))]
[RequireComponent(typeof(TargetingTool))]
public class BooleanTool : VRTool
{
    public enum BooleanOperation { Merge, Substract, Intersect }

    public BooleanOperation currentBooleanOperation;

    [SerializeField] private TargetingTool _controllerOneTargetingTool;

    private BooleanToolInputHandler _inputHandler;
    private TargetingTool _targetingTool;
    private EditableMesh _mainTarget;
    private EditableMesh _secondaryTarget;

    [SerializeField] private Material _mainTargetHoverMat;
    [SerializeField] private Material _secondaryTargetHoverMat;

    private EditableMesh MainTarget
    {
        get { return _mainTarget; }
        set
        {
            if (_mainTarget != null)
                _mainTarget.MeshMaterial = _mainTargetBaseMat;
            _mainTarget = value;
            if (_mainTarget != null)
            {
                _mainTargetBaseMat = _mainTarget.MeshMaterial;
                _mainTarget.MeshMaterial = _mainTargetHoverMat;
            }
        }
    }

    private EditableMesh SecondaryTarget
    {
        get { return _secondaryTarget; }
        set
        {
            if (_secondaryTarget != null)
                _secondaryTarget.MeshMaterial = _secondaryTargetBaseMat;
            _secondaryTarget = value;
            if (_secondaryTarget != null)
            {
                _secondaryTargetBaseMat = _secondaryTarget.MeshMaterial;
                _secondaryTarget.MeshMaterial = _secondaryTargetHoverMat;
            }
        }
    }

    private Material _mainTargetBaseMat;
    private Material _secondaryTargetBaseMat;

    void Start()
    {
        _targetingTool = transform.GetComponent<TargetingTool>();
        _inputHandler = transform.GetComponent<BooleanToolInputHandler>();
        _inputHandler.ApplyBooleanInputPressed += ApplyBoolean;
    }

    private void OnDisable()
    {
        MainTarget = null;
        SecondaryTarget = null;
    }

    // Update is called once per frame
    void Update()
    {      
        if (_targetingTool.Target != null && _targetingTool.Target.GetComponent<EditableMesh>() != null &&
            SecondaryTarget != _targetingTool.Target.GetComponent<EditableMesh>())
        {
            if (MainTarget != _targetingTool.Target.GetComponent<EditableMesh>())
                MainTarget = _targetingTool.Target.GetComponent<EditableMesh>();
        }        
        else
            MainTarget = null;
        if (_controllerOneTargetingTool.Target != null && _controllerOneTargetingTool.Target.GetComponent<EditableMesh>() != null && 
            MainTarget != _controllerOneTargetingTool.Target.GetComponent<EditableMesh>())
        {
            if (SecondaryTarget != _controllerOneTargetingTool.Target.GetComponent<EditableMesh>())
                 SecondaryTarget = _controllerOneTargetingTool.Target.GetComponent<EditableMesh>();
        }
        else
            SecondaryTarget = null;
    }

    private void ApplyBoolean()
    {
        if (MainTarget != null && SecondaryTarget != null)
        {
            switch(currentBooleanOperation)
            {
                case BooleanOperation.Substract:
                    MainTarget.Subtract(SecondaryTarget);
                    break;
                case BooleanOperation.Merge:
                    MainTarget.Merge(SecondaryTarget);
                    break;
                case BooleanOperation.Intersect:
                    MainTarget.Intersect(SecondaryTarget);
                    break;
                default:
                    break;
            }
            SceneManager.Instance.AddToUndoRedoList(MainTarget);
        }
    }
}
