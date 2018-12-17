using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace Tectrid.Primitives
{
    [Serializable]
    public class Box : Primitive
    {
        float _width = 1;
        float _height = 1;
        float _depth = 1;

        public Box(float width, float height, float depth)
        {
            _width = width <= 0 ? 1 : width;
            _height = height <= 0 ? 1 : height;
            _depth = depth <= 0 ? 1 : depth;
        }

        public override IntPtr Generate()
        {
            return DLL.GenBox_Generate(_width, _height, _depth);
        }
    }
}
