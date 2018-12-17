using System.Collections;
using System.Collections.Generic;
using UnityEngine;

struct BooleanButtonInfos
{
    public BooleanButton button;
    public Vector3 startPosition;
}

public class BooleanUI : MonoBehaviour
{
    private BooleanButtonInfos _selected;
    private BooleanButtonInfos _deselected;

    private Dictionary<BooleanTool.BooleanOperation, BooleanButtonInfos> _operationButtonsMap = new Dictionary<BooleanTool.BooleanOperation, BooleanButtonInfos>();

    private Coroutine placeRoutine = null;
    private Coroutine replaceRoutine = null;

    [SerializeField] private float _selectionYOffset;
    [SerializeField] private float _animationDuration;

    void Start ()
    {
        CreateOperationButtonsMap();
        _operationButtonsMap[BooleanTool.BooleanOperation.Merge].button.Select(null);
	}

    private void OnDisable()
    {
        if (placeRoutine != null)
            StopCoroutine(placeRoutine);
        if (replaceRoutine != null)
            StopCoroutine(replaceRoutine);
        if (_selected.button != null)
            _selected.button.transform.localPosition = new Vector3(_selected.startPosition.x, _selected.startPosition.y + _selectionYOffset, _selected.startPosition.z);
        if (_deselected.button != null)
            _deselected.button.transform.localPosition = _deselected.startPosition;
    }

    public void SelectOperation(BooleanTool.BooleanOperation operationType)
    {
        BooleanButtonInfos selection = _operationButtonsMap[operationType];
        placeRoutine = StartCoroutine(MoveButton(selection, _selectionYOffset));
        if (_selected.button != null)
        {
            replaceRoutine = StartCoroutine(MoveButton(_selected, -_selectionYOffset));
            _deselected = _selected;
            _deselected.button.Deselect();
        }
        _selected = _operationButtonsMap[operationType];
    }

    IEnumerator MoveButton(BooleanButtonInfos button, float yOffset)
    {
        Vector3 startPos = button.startPosition;
        float targetY = startPos.y + yOffset;
        float timer = 0;
        float y;
        while (timer < _animationDuration)
        {
            y = Mathf.Lerp(startPos.y, targetY, timer / _animationDuration);
            button.button.transform.localPosition = new Vector3(button.button.transform.localPosition.x, y, button.button.transform.localPosition.z);
            timer += Time.deltaTime;
            yield return null;
        }
        yield return null;
    }

    private void CreateOperationButtonsMap()
    {
            BooleanButton booleanButtonScript;
            foreach (Transform child in transform)
            {
                if ((booleanButtonScript = child.GetComponent<BooleanButton>()) != null)
                {
                    _operationButtonsMap.Add(booleanButtonScript.CurrentOperation, new BooleanButtonInfos() { button = booleanButtonScript, startPosition = child.localPosition });
                }
            }
    }
}
