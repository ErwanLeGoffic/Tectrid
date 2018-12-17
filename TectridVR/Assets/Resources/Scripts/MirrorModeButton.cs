using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class MirrorModeButton : VRUIButton
{
    [SerializeField] private TextMesh _text;

    protected override void Start()
    {
        base.Start();
        Tectrid.Parameters.SetMirrorMode(false);
        _text.text = "Off";
    }

    public override void Select(GameObject sender)
    {
        Tectrid.Parameters.SetMirrorMode(!Tectrid.Parameters.IsMirrorModeActivated());
        _text.text = Tectrid.Parameters.IsMirrorModeActivated() ? "On" : "Off";
    }
}
