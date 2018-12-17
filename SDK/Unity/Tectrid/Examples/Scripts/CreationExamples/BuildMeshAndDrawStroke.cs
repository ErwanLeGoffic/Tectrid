using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Tectrid.Examples
{
    public class BuildMeshAndDrawStroke : MonoBehaviour
    {
        EditableMesh _editableMesh;

        // Adds the editable mesh component, builds the editable mesh from the unity mesh present on the object 
        // and draw a simple stroke
        void Start()
        {
            if ((_editableMesh = gameObject.GetComponent<EditableMesh>()) == null)
                _editableMesh = gameObject.AddComponent<EditableMesh>();
            _editableMesh.Build();
            DrawStroke();
        }

        void DrawStroke()
        {
            Dictionary<Vector3, Vector3> strokes = new Dictionary<Vector3, Vector3>()
            {
                {  new Vector3(transform.position.x - 5, transform.position.y, transform.position.z - 10f),  new Vector3(5, 0, 10) },
                {  new Vector3(transform.position.x + 5, transform.position.y, transform.position.z - 10f), new Vector3(-5, 0, 10)}
            };
            StrokeDrawer.Draw(Brush.Type.Draw, _editableMesh, strokes);
        }
    }
}

