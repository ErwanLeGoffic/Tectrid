using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class DirectoryButton : VRUIButton
{
    public string directoryName;
    public FileBrowser fileBrowser;

    private Text _text;

    protected override void Start()
    {
        _text = transform.GetComponent<Text>();
        _text.text = directoryName;
        _text.color = baseColor;
    }

    public override void OnHovered()
    {
        _text.color = hoveredColor;
    }

    public override void OnUnhovered()
    {
        _text.color = baseColor;
    }

    public override void Select(GameObject sender)
    {
        fileBrowser.EnterDirectory(directoryName);
    }
}
