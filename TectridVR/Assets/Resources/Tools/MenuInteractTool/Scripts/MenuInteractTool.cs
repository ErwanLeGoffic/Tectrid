using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(MenuInteractToolInputHandler))]
[RequireComponent(typeof(TargetingTool))]
public class MenuInteractTool : MonoBehaviour {

    private TargetingTool _targettingTool;
    private MenuInteractToolInputHandler _inputManager;

    public VRUIButton Target { get; private set; }
    private VRUIButton _lastClicked = null;

    // Use this for initialization
    void Start ()
    {
        _targettingTool = transform.GetComponent<TargetingTool>();
        _inputManager = transform.GetComponent<MenuInteractToolInputHandler>();
        _inputManager.SelectInputPressed += Select;
	}

    // Update is called once per frame
    void Update ()
    {
        VRUIButton newTarget = _targettingTool.Target != null ? _targettingTool.Target.GetComponent<VRUIButton>() : null;
        if (Target != null && (newTarget == null || Target != newTarget))
        {     
            Target.OnUnhovered();
        }      
        if (Target != newTarget)
        {
            Target = newTarget;
            if (Target != null)
                Target.OnHovered();
        }
    }

    private void Select()
    {
        if (Target != null)
        {
            Target.Select(gameObject);
            _lastClicked = Target;
        }
    }
}
