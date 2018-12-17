using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ViveMenuInteractToolInputHandler : MenuInteractToolInputHandler {

    private ViveControllerInputHandler _inputProvider;

    protected override void InitInputProvider()
    {
        _inputProvider = transform.GetComponent<ViveControllerInputHandler>();
        _inputProvider.OnTriggerDown += OnSelectInputPressed;
    }

    protected override void UpdateInputs()
    {
        if (_inputProvider != null)
        {
            Inputs = new MenuInteractInputs
            {
                Select = _inputProvider.TriggerDown
            };
        }
    }
}
