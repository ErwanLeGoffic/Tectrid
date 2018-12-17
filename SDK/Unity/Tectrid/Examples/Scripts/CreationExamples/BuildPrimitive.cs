using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace Tectrid.Examples
{
    public class BuildPrimitive : MonoBehaviour
    {
        private EditableMesh _editableMesh;

        void Start()
        {
            if((_editableMesh = gameObject.GetComponent<EditableMesh>()) == null)
                _editableMesh = gameObject.AddComponent<EditableMesh>();

            if(_editableMesh != null)
            {
                _editableMesh.Build(new Tectrid.Primitives.Box(2, 2, 2));
            }
        }
    }
}
