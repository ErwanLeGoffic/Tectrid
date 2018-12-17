using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ViveSpawnToolInputHandler : SpawnToolInputHandler
{
    private ViveControllerInputHandler _inputProvider;

    [SerializeField] private float _timeBetweenObjectDistanceChangesInputs = 0.2f;
    [SerializeField] private float _timeBetweenObjectSizeChangesInputs = 0.2f;

    private float _lastObjectDistanceChangeTime = 0;
    private float _lastObjectSizeChangeTime = 0;

    protected override void InitInputProvider()
    {
        _inputProvider = transform.GetComponent<ViveControllerInputHandler>();
        _inputProvider.OnTriggerDown += OnSpawnButtonPressed;
    }

    protected override void UpdateInputs()
    {
        if (_inputProvider != null)
        {
            Inputs = new SpawnInputs
            {
                Spawn = _inputProvider.TriggerDown,
                ObjectDistanceChange = ComputeObjectDistanceChangeInput(),
                ObjectSizeChange = ComputeObjectSizeChangeInput()
            };
            if (Inputs.ObjectDistanceChange != 0)
                OnObjectDistanceChangeInputPressed();
            if (Inputs.ObjectSizeChange != 0)
                OnObjectSizeChangeInputPressed();
        }
    }

    protected override float ComputeObjectDistanceChangeInput()
    {
        if (_inputProvider.PadStay && Time.time - _lastObjectDistanceChangeTime > _timeBetweenObjectDistanceChangesInputs)
        {
            _lastObjectDistanceChangeTime = Time.time;
            if (_inputProvider.PadTouchAngleDeg > 45 && _inputProvider.PadTouchAngleDeg < 135)
                return (1);
            else if (_inputProvider.PadTouchAngleDeg < -45 && _inputProvider.PadTouchAngleDeg > -135)
                return (-1);
        }
        return 0;
    }

    protected override float ComputeObjectSizeChangeInput()
    {
        if (_inputProvider.PadStay && Time.time - _lastObjectSizeChangeTime > _timeBetweenObjectSizeChangesInputs)
        {
            _lastObjectSizeChangeTime = Time.time;
            if (_inputProvider.PadTouchAngleDeg < 45 && _inputProvider.PadTouchAngleDeg > -45)
                return (1);
            else if (_inputProvider.PadTouchAngleDeg > 135 || _inputProvider.PadTouchAngleDeg < -135)
                return (-1);
        }
        return 0;
    }


}
