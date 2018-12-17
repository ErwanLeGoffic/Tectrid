using UnityEngine;
using UnityEngine.UI;

public class UIToggleMirror : MonoBehaviour
{
	void Start()
    {
        Toggle toggle = gameObject.GetComponent<Toggle>();
        if(toggle != null)
            toggle.isOn = Tectrid.Parameters.IsMirrorModeActivated();
    }
}
