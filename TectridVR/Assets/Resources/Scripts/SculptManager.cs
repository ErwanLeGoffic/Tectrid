using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Tectrid;

public class SculptManager : ModeManager
{
    private SculptUI _sculptUI;
    private SculptTool _sculptingTool;

    private void Start()
    {
        _sculptUI = _controllerOneUi.GetComponent<SculptUI>();
        _sculptingTool = (SculptTool)_tool;
    }

    public void SelectBrush(Brush.Type brushType)
    {
        _sculptUI.SelectBrush(brushType);
        _sculptingTool.SelectBrush(brushType);
        _sculptingTool.Erasing = false;
    }

    public void EnterEraseMode()
    {
        _sculptUI.SelectEraseBrush();
        _sculptingTool.Erasing = true;
    }
}
