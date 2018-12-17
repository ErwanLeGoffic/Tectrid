using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ViveTeleportToolInputHandler : TeleportToolInputHandler
{
    private ViveControllerInputHandler _inputProvider;

    protected override void InitInputProvider()
    {
        _inputProvider = transform.GetComponent<ViveControllerInputHandler>();
        _inputProvider.OnTriggerDown += OnTeleportInputPressed;
    }

    protected override void UpdateInputs()
    {
        if (_inputProvider != null)
        {
            Inputs = new TeleportInputs
            {
                Teleport = _inputProvider.TriggerDown
            };
        }
    }
}
