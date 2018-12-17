using UnityEngine;
using UnityEngine.EventSystems; // Required when using Event data.

public class UILockInput : MonoBehaviour, ISelectHandler, IDeselectHandler
{
	//Do this when the selectable UI object is selected.
	public void OnSelect(BaseEventData eventData)
	{
		Debug.Log(this.gameObject.name + " was selected");
		Main.uiIsInUse = true;
	}

	//Do this when the selectable UI object is selected.
	public void OnDeselect(BaseEventData eventData)
	{
		Debug.Log(this.gameObject.name + " was de-selected");
		Main.uiIsInUse = false;
	}
}
