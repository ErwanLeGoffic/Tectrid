using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SelectionTypeButton : VRUIButton
{
    [SerializeField] private TextMesh _text;

    public override void Select(GameObject sender)
    {
        ModeSwitchManager.SwitchSelectionType();
        _text.text = ModeSwitchManager.selectionType.ToString();
    }
}
