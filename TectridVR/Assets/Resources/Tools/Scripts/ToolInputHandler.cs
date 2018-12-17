using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public abstract class ToolInputHandler : MonoBehaviour
{
    protected virtual void Start()
    {
        InitInputProvider();
    }

    protected virtual void Update()
    {
        UpdateInputs();
    }

    protected abstract void InitInputProvider();
    protected abstract void UpdateInputs();
}
