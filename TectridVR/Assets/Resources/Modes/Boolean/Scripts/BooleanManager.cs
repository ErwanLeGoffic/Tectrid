using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BooleanManager : ModeManager
{
    private BooleanUI _booleanUI;
    private BooleanTool _booleanTool;

    private void Start()
    {
        _booleanUI = _controllerOneUi.GetComponent<BooleanUI>();
        _booleanTool = (BooleanTool)_tool;
    }

    public void SelectOperation(BooleanTool.BooleanOperation operation)
    {
        _booleanTool.currentBooleanOperation = operation;
        _booleanUI.SelectOperation(operation);
    }
}
