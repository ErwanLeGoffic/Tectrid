using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public abstract class BooleanToolInputHandler : ToolInputHandler
{
    private BooleanInputs _Inputs;
    public BooleanInputs Inputs
    {
        get { return _Inputs; }
        protected set { _Inputs = value; }
    }

    public event Action ApplyBooleanInputPressed;

    protected abstract override void InitInputProvider();
    protected abstract override void UpdateInputs();

    protected void OnApplyBooleanInputPressed()
    {
        if (ApplyBooleanInputPressed != null)
            ApplyBooleanInputPressed();
    }
}


public struct BooleanInputs
{
    public bool Apply;
}
