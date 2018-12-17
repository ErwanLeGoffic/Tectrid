using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public abstract class TeleportToolInputHandler : ToolInputHandler
{
    private TeleportInputs _Inputs;
    public TeleportInputs Inputs
    {
        get { return _Inputs; }
        protected set { _Inputs = value; }
    }

    public event Action TeleportInputPressed;

    protected abstract override void InitInputProvider();
    protected abstract override void UpdateInputs();

    protected void OnTeleportInputPressed()
    {
        if (TeleportInputPressed != null)
            TeleportInputPressed();        
    }
}

public struct TeleportInputs
{
    public bool Teleport;
}