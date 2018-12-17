using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class ScrollButton : VRUIButton {

    public enum Direction { Up, Down}

    [SerializeField] private Direction _direction;
    [SerializeField] private Scrollbar _scrollBar;
    
    private bool _hovered;
    private Image _image;
    private float scrollSpeed = 1;

    protected override void Start()
    {
        _image = transform.GetComponent<Image>();
        _image.color = baseColor;
    }

    private void Update()
    {
        if (_hovered)
        {
            int direction = _direction == Direction.Down ? -1 : 1;
            _scrollBar.value += scrollSpeed * Time.deltaTime * direction * _scrollBar.size;
        }
    }
    public override void OnHovered()
    {
        _hovered = true;
        _image.color = hoveredColor;
    }

    public override void OnUnhovered()
    {
        _hovered = false;
        _image.color = baseColor;
    }
}
