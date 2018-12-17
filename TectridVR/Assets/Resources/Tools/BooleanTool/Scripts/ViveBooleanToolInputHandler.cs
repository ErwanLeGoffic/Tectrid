using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ViveBooleanToolInputHandler : BooleanToolInputHandler
{
    private ViveControllerInputHandler _inputProvider;

    protected override void InitInputProvider()
    {
        _inputProvider = transform.GetComponent<ViveControllerInputHandler>();
        _inputProvider.OnTriggerDown += OnApplyBooleanInputPressed;
    }

    protected override void UpdateInputs()
    {
        if (_inputProvider != null)
        {
            Inputs = new BooleanInputs
            {
                Apply = _inputProvider.TriggerDown
            };
        }
    }
}
