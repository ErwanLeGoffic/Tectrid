using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class VRUIButton : MonoBehaviour
{
    [SerializeField]
    protected Color baseColor, hoveredColor;

    protected MeshRenderer _renderer;

    protected virtual void Start()
    {
        _renderer = transform.GetComponent<MeshRenderer>();
        _renderer.material.color = baseColor;
    }

    public virtual void OnHovered()
    {
        _renderer.material.color = hoveredColor;
    }

    public virtual void OnUnhovered()
    {
        _renderer.material.color = baseColor;
    }

    public virtual void Select(GameObject sender)
    {

    }

    public virtual void Deselect()
    {

    }
}
