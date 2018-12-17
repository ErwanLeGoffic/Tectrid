using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class EraseButton : VRUIButton
{
    private bool _selected;
    [SerializeField] private Color _selectedColor;
    [SerializeField] private SculptManager _sculptManager;

    private void Awake()
    {
        _renderer = transform.GetComponent<MeshRenderer>();
        _renderer.material.color = baseColor;
    }

    public override void OnHovered()
    {
        if (!_selected)
            _renderer.material.color = hoveredColor;
    }

    public override void OnUnhovered()
    {
        if (!_selected)
            _renderer.material.color = baseColor;
    }

    public override void Select(GameObject sender)
    {
        if (!_selected)
        {
            _selected = true;
            _renderer.material.color = _selectedColor;
            _sculptManager.EnterEraseMode();
        }
    }

    public override void Deselect()
    {
        _selected = false;
        _renderer.material.color = baseColor;      
    }
}
