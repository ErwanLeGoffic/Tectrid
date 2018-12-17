using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(ViveControllerInputHandler))]
public class ViveSculptToolInputHandler : SculptToolInputHandler
{
    private ViveControllerInputHandler _inputProvider;

    [SerializeField] private ViveControllerInputHandler _controllerOneInputHandler;
    [SerializeField] private float _timeBetweenBrushRadiusChangesInputs = 0.2f;
    [SerializeField] private float _timeBetweenBrushStrengthChangesInputs = 0.2f;

    private float _lastBrushRadiusChangeTime = 0;
    private float _lastBrushStrengthChangeTime = 0;

    protected override void InitInputProvider()
    {
        _inputProvider = transform.GetComponent<ViveControllerInputHandler>();
        _inputProvider.OnTriggerDown += OnStrokeStartInputPressed;
        _inputProvider.OnTriggerStay += OnStrokeUpdateInputPressed;
        _inputProvider.OnTriggerUp += OnStrokeStopInputPressed;
    }

    protected override void UpdateInputs()
    {
        if (_inputProvider != null)
        {
            Inputs = new SculptInputs
            {
                StartStroke = _inputProvider.TriggerDown,
                UpdateStroke = _inputProvider.TriggerStay,
                StopStroke = _inputProvider.TriggerUp,
                Undo = ComputeUndoInput(),
                Redo = ComputeRedoInput(),
                BrushRadiusChange = ComputeBrushRadiusChangeInput(),
                BrushStrengthChange = ComputeBrushStrengthChangeInput()
            };
            if (Inputs.BrushRadiusChange != 0)
                OnBrushRadiusChangeInputPressed();
            if (Inputs.BrushStrengthChange != 0)
                OnBrushStrengthChangeInputPressed();
            if (Inputs.Undo)
                OnUndoInputPressed();
            if (Inputs.Redo)
                OnRedoInputPressed();
        }
    }

    private bool ComputeUndoInput()
    {
        if (_controllerOneInputHandler.PadDown && !(_controllerOneInputHandler.TriggerDown || _controllerOneInputHandler.TriggerStay))
        {
            if (_controllerOneInputHandler.PadTouchPosition.x < 0)
                return true;
        }
        return false;
    }

    private bool ComputeRedoInput()
    {
        if (_controllerOneInputHandler.PadDown && !(_controllerOneInputHandler.TriggerDown || _controllerOneInputHandler.TriggerStay))
        {
            if (_controllerOneInputHandler.PadTouchPosition.x > 0)
                return true;
        }
        return false;
    }

    protected override float ComputeBrushRadiusChangeInput()
    {
        if (_inputProvider.PadStay && Time.time - _lastBrushRadiusChangeTime > _timeBetweenBrushRadiusChangesInputs)
        {
            _lastBrushRadiusChangeTime = Time.time;
            if (_inputProvider.PadTouchAngleDeg > 45 && _inputProvider.PadTouchAngleDeg < 135)
                return (1);
            else if (_inputProvider.PadTouchAngleDeg < -45 && _inputProvider.PadTouchAngleDeg > -135)
                return (-1);
        }
        return 0;
    }

    protected override float ComputeBrushStrengthChangeInput()
    {
        if (_inputProvider.PadStay && Time.time - _lastBrushStrengthChangeTime > _timeBetweenBrushStrengthChangesInputs)
        {
            _lastBrushStrengthChangeTime = Time.time;
            if (_inputProvider.PadTouchAngleDeg < 45 && _inputProvider.PadTouchAngleDeg > -45)
                return (1);
            else if (_inputProvider.PadTouchAngleDeg > 135 || _inputProvider.PadTouchAngleDeg < -135)
                return (-1);
        }
        return 0;
    }
}
