using System;
using System.Collections;
using System.Collections.Generic;
using Tectrid;
using UnityEngine;

[RequireComponent(typeof(SteamVR_ControllerManager))]
public class ScaleTool : MonoBehaviour
{
    private GameObject _controllerOne;
    private GameObject _controllerTwo;
    private ViveControllerInputHandler _controllerOneInputHandler;
    private ViveControllerInputHandler _controllerTwoInputHandler;

    private TargetingTool _controllerOneTargettingTool;
    private TargetingTool _controllerTwoTargettingTool;

    private float _prevDistance;
    private Transform _target;

    void Start()
    {
        _controllerOne = transform.GetComponent<SteamVR_ControllerManager>().left;
        _controllerTwo = transform.GetComponent<SteamVR_ControllerManager>().right;
        _controllerOneInputHandler = _controllerOne.GetComponent<ViveControllerInputHandler>();
        _controllerTwoInputHandler = _controllerTwo.GetComponent<ViveControllerInputHandler>();
        _controllerOneTargettingTool = _controllerOne.GetComponent<TargetingTool>();
        _controllerTwoTargettingTool = _controllerTwo.GetComponent<TargetingTool>();
        _prevDistance = Vector3.Distance(_controllerOne.transform.position, _controllerTwo.transform.position);
    }

    void Update ()
    {
        float distance = Vector3.Distance(_controllerOne.transform.position, _controllerTwo.transform.position);
        if (_controllerOneInputHandler.GripStay && _controllerTwoInputHandler.GripStay && _target != null)
        {
            float scaleFactor = distance - _prevDistance;
            _target.localScale += new Vector3(scaleFactor, scaleFactor, scaleFactor);
        }
        else
        {
            SetTarget(_controllerOneTargettingTool.Target, _controllerTwoTargettingTool.Target);
        }
        _prevDistance = distance;
	}

    private void SetTarget(GameObject t1, GameObject t2)
    {
        _target = null;
        if (t1 != null && t2 != null && t1.GetComponent<EditableMesh>() != null && t2.GetComponent<EditableMesh>() != null)
        {
            if (ModeSwitchManager.selectionType == ModeSwitchManager.SelectionType.Object)
            {
                if (t1 == t2)
                    _target = t1.transform;
            }
            else
            {
                Transform p1 = GetRoot(t1.transform);
                Transform p2 = GetRoot(t2.transform);
                if (p1 == p2)
                {
                    _target = p1;
                }
            }
        }
    }

    private Transform GetRoot(Transform go)
    {
        while (go.transform.parent != null)
        {
            go = go.transform.parent;
        }
        return go;
    }
}
