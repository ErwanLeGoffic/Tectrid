using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Tectrid;


struct BrushButtonInfos
{
    public BrushButton Button;
    public Vector3 StartPosition;
}

public class SculptUI : MonoBehaviour
{
    [SerializeField] private GameObject _brushObjectsContainer;

    private GameObject _eraseButton = null;
    private bool _erasing;
   
    private BrushButtonInfos _selected;
    private BrushButtonInfos _deselected;

    private Dictionary<Brush.Type, BrushButtonInfos> _brushButtonsMap = new Dictionary<Brush.Type, BrushButtonInfos>();

    private Coroutine placeRoutine = null;
    private Coroutine replaceRoutine = null;

    [SerializeField] private float _selectionYOffset;
    [SerializeField] private float _animationDuration;

    void Start ()
    {
        CreateBrushButtonsMap();
        _brushButtonsMap[Brush.Type.Draw].Button.Select(null);
    }

    private void OnEnable()
    {
        if (_erasing)
        {
            _brushButtonsMap[Brush.Type.Draw].Button.Select(null);
        }
    }

    private void OnDisable()
    {
        if (placeRoutine != null)
            StopCoroutine(placeRoutine);
        if (replaceRoutine != null)
            StopCoroutine(replaceRoutine);
        if (_selected.Button != null)
            _selected.Button.transform.localPosition = new Vector3(_selected.StartPosition.x, _selected.StartPosition.y + _selectionYOffset, _selected.StartPosition.z);
        if (_deselected.Button != null)
            _deselected.Button.transform.localPosition = _deselected.StartPosition;
    }

    public void SelectEraseBrush()
    {
        StartCoroutine(MoveButton(_selected, -_selectionYOffset));
        _selected.Button.Deselect();
        _selected.Button = null;
        _erasing = true;
    }

    public void SelectBrush(Brush.Type brushType)
    {
        BrushButtonInfos selection = _brushButtonsMap[brushType];
        placeRoutine = StartCoroutine(MoveButton(selection, _selectionYOffset));
        if (_selected.Button != null)
        {
            replaceRoutine = StartCoroutine(MoveButton(_selected, -_selectionYOffset));
            _deselected = _selected;
            _deselected.Button.Deselect();
        }
        if (_erasing)
        {
            _eraseButton.GetComponent<EraseButton>().Deselect();
            _erasing = false;
        }
        _selected = _brushButtonsMap[brushType];
    }

    IEnumerator MoveButton(BrushButtonInfos button, float yOffset)
    {
        Vector3 startPos = button.StartPosition;
        float targetY = startPos.y + yOffset;
        float timer = 0;
        float y;
        while (timer < _animationDuration)
        {
            y = Mathf.Lerp(startPos.y, targetY, timer / _animationDuration);
            button.Button.transform.localPosition = new Vector3(button.Button.transform.localPosition.x, y, button.Button.transform.localPosition.z);
            timer += Time.deltaTime;
            yield return null;
        }
        yield return null;
    }

    private void CreateBrushButtonsMap()
    {
        if (_brushObjectsContainer != null)
        {
            BrushButton brushButtonScript;
            foreach (Transform child in _brushObjectsContainer.transform)
            {
                if ((brushButtonScript = child.GetComponent<BrushButton>()) == null)
                {
                    if (child.GetComponent<EraseButton>() != null)
                    {
                        _eraseButton = child.gameObject;
                    }
                }
                else
                {
                    _brushButtonsMap.Add(brushButtonScript.BrushType, new BrushButtonInfos() { Button = brushButtonScript, StartPosition = child.localPosition });
                }
            }
        }
    }
}
