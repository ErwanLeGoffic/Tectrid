using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class OpenBrowserButton : VRUIButton
{
    [SerializeField] private GameObject _fileBrowser;

    public override void Select(GameObject sender)
    {
        if (!_fileBrowser.activeSelf)
            _fileBrowser.SetActive(true);
        else
            _fileBrowser.GetComponent<FileBrowser>().ResetPosition();
    }
}
