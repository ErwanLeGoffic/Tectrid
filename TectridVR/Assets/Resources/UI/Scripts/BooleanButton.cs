using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BooleanButton : VRUIButton {

    [SerializeField] private BooleanTool.BooleanOperation _CurrentOperation;
    public BooleanTool.BooleanOperation CurrentOperation
    {
        get { return _CurrentOperation; }
    }

    private bool _selected;


    [SerializeField] private BooleanManager _booleanManager;
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
            _booleanManager.SelectOperation(_CurrentOperation);
        }
    }

    public override void Deselect()
    {
        _selected = false;
        _renderer.material.color = baseColor;
    }
}
