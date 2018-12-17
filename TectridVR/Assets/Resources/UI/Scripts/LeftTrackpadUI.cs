using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LeftTrackpadUI : MonoBehaviour {

    [SerializeField] private ViveControllerInputHandler _inputHandler;
    [SerializeField] private GameObject _transformUI;
    [SerializeField] private GameObject _undoRedoUI;

	// Use this for initialization
	void Start ()
    {
        _inputHandler.OnTriggerDown += ShowTransformUI;
        _inputHandler.OnTriggerUp += ShowUndoRedoUI;
        ShowUndoRedoUI();
    }
	
	private void ShowTransformUI()
    {
        _transformUI.SetActive(true);
        _undoRedoUI.SetActive(false);
    }

    private void ShowUndoRedoUI()
    {
        _transformUI.SetActive(false);
        _undoRedoUI.SetActive(true);
    }
}
