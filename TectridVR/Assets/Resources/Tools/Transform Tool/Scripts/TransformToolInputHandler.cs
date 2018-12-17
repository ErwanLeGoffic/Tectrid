using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public abstract class TransformToolInputHandler : ToolInputHandler
{
    private TransformInputs _Inputs;
    public TransformInputs Inputs
    {
        get { return _Inputs; }
        protected set { _Inputs = value; }
    }

    public event Action HoldButtonPressed;
    public event Action HoldButtonUnpressed;
    public event Action ObjectDistanceChanged;

    protected abstract override void InitInputProvider();
    protected abstract override void UpdateInputs();
    protected abstract int ComputeObjectDistanceChange();

    protected void OnHoldButtonPressed()
    {
        if (HoldButtonPressed != null)
            HoldButtonPressed();
    }

    protected void OnHoldButtonUnpressed()
    {
        if (HoldButtonUnpressed != null)
            HoldButtonUnpressed();
    }

    protected void OnObjectDistanceChanged()
    {
        if (ObjectDistanceChanged != null)
            ObjectDistanceChanged();
    }
}

public struct TransformInputs
{
    public bool Hold;
    public int ObjectDistanceChange;
}