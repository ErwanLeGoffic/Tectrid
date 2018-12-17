using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace Tectrid.Primitives
{
    [Serializable]
    public class Sphere : Primitive
    {
        float _radius = 10;

        public Sphere(float radius)
        {
            _radius = radius <= 0 ? 1 : radius;
        }

        public override IntPtr Generate()
        {
            return DLL.GenSphere_Generate(_radius);
        }
    }
}
