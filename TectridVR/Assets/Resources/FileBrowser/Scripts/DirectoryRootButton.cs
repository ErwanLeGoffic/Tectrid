using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class DirectoryRootButton : VRUIButton
{
    [SerializeField] private FileBrowser fileBrowser;

    private Image _image;

    protected override void Start()
    {
        _image = transform.GetComponent<Image>();
        _image.color = baseColor;
    }

    public override void OnHovered()
    {
        _image.color = hoveredColor;
    }

    public override void OnUnhovered()
    {
        _image.color = baseColor;
    }

    public override void Select(GameObject sender)
    {
        fileBrowser.Back();
    }
}
