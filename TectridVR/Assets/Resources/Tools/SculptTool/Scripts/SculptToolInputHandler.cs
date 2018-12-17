using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public abstract class SculptToolInputHandler : ToolInputHandler {

    private SculptInputs _Inputs;
    public SculptInputs Inputs
    {
        get { return _Inputs; }
        protected set { _Inputs = value; }
    }

    public event Action StrokeStartInputPressed;
    public event Action StrokeUpdateInputPressed;
    public event Action StrokeStopInputPressed;
    public event Action UndoInputPressed;
    public event Action RedoInputPressed;
    public event Action BrushRadiusChangeInputPressed;
    public event Action BrushStrengthChangeInputPressed;

    protected abstract override void InitInputProvider();
    protected abstract override void UpdateInputs();
    protected abstract float ComputeBrushRadiusChangeInput();
    protected abstract float ComputeBrushStrengthChangeInput();

    protected void OnStrokeStartInputPressed()
    {
        if (StrokeStartInputPressed != null)
            StrokeStartInputPressed();
    }

    protected void OnStrokeUpdateInputPressed()
    {
        if (StrokeUpdateInputPressed != null)
            StrokeUpdateInputPressed();
    }

    protected void OnStrokeStopInputPressed()
    {
        if (StrokeStopInputPressed != null)
            StrokeStopInputPressed();
    }

    protected void OnUndoInputPressed()
    {
        if (UndoInputPressed != null)
            UndoInputPressed();
    }

    protected void OnRedoInputPressed()
    {
        if (RedoInputPressed != null)
            RedoInputPressed();
    }

    protected void OnBrushRadiusChangeInputPressed()
    {
        if (BrushRadiusChangeInputPressed != null)
            BrushRadiusChangeInputPressed();
    }

    protected void OnBrushStrengthChangeInputPressed()
    {
        if (BrushStrengthChangeInputPressed != null)
            BrushStrengthChangeInputPressed();
    }
}

public struct SculptInputs
{
    public bool StartStroke;
    public bool UpdateStroke;
    public bool StopStroke;
    public bool Undo;
    public bool Redo;
    public float BrushRadiusChange;
    public float BrushStrengthChange;
}