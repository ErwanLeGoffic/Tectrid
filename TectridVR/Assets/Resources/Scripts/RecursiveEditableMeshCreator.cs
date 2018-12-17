using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Tectrid;

public class RecursiveEditableMeshCreator : MonoBehaviour {

    private void Start()
    {
        List<Transform> transforms = new List<Transform>();
        GetChilds(transform, transforms);
        foreach (Transform t in transforms)
        {
            EditableMesh eMesh = t.gameObject.AddComponent<EditableMesh>();
            eMesh.Build();
        }
    }

    void GetChilds(Transform t, List<Transform> transforms)
    {
        foreach(Transform child in t)
        {
            if (child.GetComponent<MeshFilter>() != null)
            {
                transforms.Add(child);
            }
            GetChilds(child, transforms);
        }
    }
}
