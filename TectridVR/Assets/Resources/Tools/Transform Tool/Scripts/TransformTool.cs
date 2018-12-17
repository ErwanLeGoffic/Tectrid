using System;
using System.Collections;
using System.Collections.Generic;
using Tectrid;
using UnityEngine;

[RequireComponent(typeof(TransformToolInputHandler))]
[RequireComponent(typeof(TargetingTool))]
public class TransformTool : VRTool
{
    public Transform Target { get; private set; }
    private TransformToolInputHandler _inputManager;

    [SerializeField] private float _objectDistanceChangePerInput = 0.2f;

    private Transform _prevParent;
    private bool _transforming;
    private TargetingTool _targettingTool;

    void Start()
    {
        Target = null;
        _inputManager = transform.GetComponent<TransformToolInputHandler>();
        _inputManager.HoldButtonPressed += Hold;
        _inputManager.HoldButtonUnpressed += Release;
        _inputManager.ObjectDistanceChanged += MoveObject;
        _targettingTool = transform.GetComponent<TargetingTool>();
    }


    void Update()
    {
        if (!_transforming)
        {
            // Check that the object has no parent, to avoid bugs with spawntool (spawntool works with parenting system to place the object too)
            if (_targettingTool.Target != null && _targettingTool.tag != "Spawning" && _targettingTool.Target.GetComponent<EditableMesh>() != null)
            {
                Target = _targettingTool.Target.transform;
                if (ModeSwitchManager.selectionType == ModeSwitchManager.SelectionType.Hierarchy)
                {
                    while (Target.parent != null)
                        Target = Target.parent;
                }            
            }
            else if (_targettingTool.Target == null)
                Target = null;
        }
    }

    #region Input Events Handler

    void Hold()
    {
        if (Target != null)
        {
            _prevParent = Target.parent;
            Target.SetParent(transform);
            _transforming = true;
        }
    }

    void Release()
    {
        if (Target != null)
        {
            Target.SetParent(_prevParent);
        }
        _transforming = false;
    }
    
    void MoveObject()
    {
        if (Target != null)
        {
            if (_inputManager.Inputs.ObjectDistanceChange > 0)
            {
                Target.transform.position += transform.forward.normalized * _objectDistanceChangePerInput;
            }
            else
            {
                Target.transform.position -= transform.forward.normalized * _objectDistanceChangePerInput;
            }
        }
    }

    #endregion // !  Input Events Handler

}
