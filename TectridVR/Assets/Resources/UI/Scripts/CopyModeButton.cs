using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CopyModeButton : VRUIButton {


    public override void Select(GameObject sender)
    {
        SpawnTool spawnTool = sender.GetComponent<SpawnTool>();
        if (spawnTool != null)
        {
            spawnTool.Copying = true;
        }
    }
}
