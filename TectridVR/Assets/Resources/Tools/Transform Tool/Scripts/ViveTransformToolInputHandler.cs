using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ViveTransformToolInputHandler : TransformToolInputHandler
{
    private ViveControllerInputHandler _inputProvider;

    [SerializeField] private float _timeBetweenObjectDistanceChanges = 0.1f;

    private float _lastObjectDistanceChangeTime = 0;

    protected override void InitInputProvider()
    {
        _inputProvider = transform.GetComponent<ViveControllerInputHandler>();
        _inputProvider.OnTriggerDown += OnHoldButtonPressed;
        _inputProvider.OnTriggerUp += OnHoldButtonUnpressed;
    }

    protected override void UpdateInputs()
    {
        Inputs = new TransformInputs
        {
            Hold = _inputProvider.TriggerDown || _inputProvider.TriggerStay,
            ObjectDistanceChange = ComputeObjectDistanceChange()     
        };
        if (Inputs.ObjectDistanceChange != 0)
            OnObjectDistanceChanged();
    }

    protected override int ComputeObjectDistanceChange()
    {
        if ((_inputProvider.PadDown || _inputProvider.PadStay) && (_inputProvider.TriggerDown || _inputProvider.TriggerStay) && Time.time - _lastObjectDistanceChangeTime > _timeBetweenObjectDistanceChanges)
        {
            _lastObjectDistanceChangeTime = Time.time;
            if (_inputProvider.PadTouchPosition.y < 0)
                return -1;
            else
                return 1;
          
        }
        return 0;
    }
}
