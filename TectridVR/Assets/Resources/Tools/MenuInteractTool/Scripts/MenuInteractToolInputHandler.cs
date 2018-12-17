using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public abstract class MenuInteractToolInputHandler : ToolInputHandler
{
    private MenuInteractInputs _Inputs;
    public MenuInteractInputs Inputs
    {
        get { return _Inputs; }
        protected set { _Inputs = value; }
    }

    public event Action SelectInputPressed;

    protected abstract override void InitInputProvider();
    protected abstract override void UpdateInputs();

    protected void OnSelectInputPressed()
    {
        if (SelectInputPressed != null)
            SelectInputPressed();
    }
}
 
public struct MenuInteractInputs
{
    public bool Select;
}
