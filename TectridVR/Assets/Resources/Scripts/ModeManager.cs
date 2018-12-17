using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ModeManager : MonoBehaviour
{
    [SerializeField]
    protected GameObject _controllerOneUi;
    [SerializeField]
    protected GameObject _controllerTwoUi;
    [SerializeField]
    protected VRTool _tool;

    public void Activate(bool activate)
    {
        _controllerTwoUi.SetActive(activate);
        _controllerOneUi.SetActive(activate);
        _tool.enabled = activate;
    }
}
