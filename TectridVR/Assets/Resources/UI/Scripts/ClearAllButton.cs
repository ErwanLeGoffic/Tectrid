using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ClearAllButton : VRUIButton
{
    [SerializeField] private GameObject _clearConfirmationUI;

    public override void Select(GameObject sender)
    {
        _clearConfirmationUI.SetActive(true);
    }
}
