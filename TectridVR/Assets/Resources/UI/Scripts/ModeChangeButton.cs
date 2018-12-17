using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ModeChangeButton : VRUIButton
{
    [SerializeField] ModeSwitchManager _modeSwitchManager;
    [SerializeField] ModeSwitchManager.Modes _mode;
 
    public override void Select(GameObject sender)
    {
        _modeSwitchManager.Mode = _mode;
    }
}
