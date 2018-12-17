using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Tectrid.Examples
{
    // Simple Example of how you can implement strokes
    public class StrokeDrawer
    {
        public static void Draw(Brush.Type brush, EditableMesh editableMesh, Dictionary<Vector3, Vector3> strokes)
        {
            editableMesh.StartStroke(brush);
            foreach(var item in strokes)
            {
                editableMesh.UpdateStroke(brush, item.Key, item.Value.normalized, float.MaxValue,0.1f, 0.8f);
            }
            editableMesh.StopStroke();
        }
    }
}
