using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BrushButton : VRUIButton
{
    [SerializeField] private Tectrid.Brush.Type _BrushType;
    public Tectrid.Brush.Type BrushType
    {
        get { return _BrushType; }
    }

    private bool _selected;


    [SerializeField] private SculptManager _sculptManager;
    [SerializeField] private Color _selectedColor;

    private void Awake()
    {
        _renderer = transform.GetComponent<MeshRenderer>();
        _renderer.material.color = baseColor;
    }

    protected override void Start()
    {
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
            _sculptManager.SelectBrush(_BrushType);
        }
    }

    public override void Deselect()
    {
        _selected = false;
        _renderer.material.color = baseColor;
    }
}
