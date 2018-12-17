using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public abstract class SpawnToolInputHandler : ToolInputHandler
{
    private SpawnInputs _Inputs;
    public SpawnInputs Inputs
    {
        get { return _Inputs; }
        protected set { _Inputs = value; }
    }

    public event Action SpawnButtonPressed;
    public event Action ObjectDistanceChangeInputPressed;
    public event Action ObjectSizeChangeInputPressed;

    protected abstract override void InitInputProvider();
    protected abstract override void UpdateInputs();
    protected abstract float ComputeObjectDistanceChangeInput();
    protected abstract float ComputeObjectSizeChangeInput();

    protected void OnSpawnButtonPressed()
    {
        if (SpawnButtonPressed != null)
            SpawnButtonPressed();
    }

    protected void OnObjectDistanceChangeInputPressed()
    {
        if (ObjectDistanceChangeInputPressed != null)
            ObjectDistanceChangeInputPressed();
    }

    protected void OnObjectSizeChangeInputPressed()
    {
        if (ObjectSizeChangeInputPressed != null)
            ObjectSizeChangeInputPressed();
    }
}

public struct SpawnInputs
{
    public bool Spawn;
    public float ObjectDistanceChange;
    public float ObjectSizeChange;
}
