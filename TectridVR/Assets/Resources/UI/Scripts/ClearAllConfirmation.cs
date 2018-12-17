using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Tectrid;

public class ClearAllConfirmation : VRMenu
{
    public override void Confirm()
    {
       for (int i = 0; i < SceneManager.Instance.EditableMeshes.Count; i++)
        {
            if (SceneManager.Instance.EditableMeshes[i].gameObject.tag != "Spawning")
            Destroy(SceneManager.Instance.EditableMeshes[i].gameObject);
        }
        gameObject.SetActive(false);
    }
}
