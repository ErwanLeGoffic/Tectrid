using System;
using System.Collections;
using System.Collections.Generic;
using Tectrid;
using UnityEngine;

public class RotateTool : MonoBehaviour
{
    private GameObject _controllerOne;
    private GameObject _controllerTwo;
    private ViveControllerInputHandler _controllerOneInputHandler;
    private ViveControllerInputHandler _controllerTwoInputHandler;

    private TargetingTool _controllerOneTargetingTool;
    private Vector3 _prevPosition;

    private Transform _target;

    void Start()
    {
        _controllerOne = transform.GetComponent<SteamVR_ControllerManager>().left;
        _controllerTwo = transform.GetComponent<SteamVR_ControllerManager>().right;
        _controllerOneInputHandler = _controllerOne.GetComponent<ViveControllerInputHandler>();
        _controllerTwoInputHandler = _controllerTwo.GetComponent<ViveControllerInputHandler>();
        _controllerOneTargetingTool = _controllerOne.gameObject.GetComponent<TargetingTool>();
        _prevPosition = _controllerOne.transform.position;
    }

    // Update is called once per frame
    void Update()
    {
        if (_controllerOneInputHandler.GripStay && !_controllerTwoInputHandler.GripStay && _target != null)
        {
            _target.transform.Rotate(new Vector3(_controllerOne.transform.position.y - _prevPosition.y, _prevPosition.x - _controllerOne.transform.position.x) * 350, Space.World);
        }
        else
        {
            _target = null;
            if (_controllerOneTargetingTool.Target != null && _controllerOneTargetingTool.Target.GetComponent<EditableMesh>() != null)
            {
                _target = _controllerOneTargetingTool.Target.transform;
                if (ModeSwitchManager.selectionType == ModeSwitchManager.SelectionType.Hierarchy)
                {
                    while (_target.parent != null)
                        _target = _target.parent;
                }
            }
        }
        _prevPosition = _controllerOne.transform.position;
    }
}
