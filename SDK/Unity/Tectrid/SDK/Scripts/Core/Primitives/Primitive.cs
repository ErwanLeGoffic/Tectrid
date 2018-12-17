using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

namespace Tectrid.Primitives
{
    public abstract class Primitive
    {
        private delegate Primitive BuildPrimitiveDelegate(List<float> parameters);

        private static readonly Dictionary<Primitive.Types, BuildPrimitiveDelegate> PrimitiveBuildMap = new Dictionary<Primitive.Types, BuildPrimitiveDelegate>
        {
            {Types.Box, BuildBox },
            {Types.Sphere, BuildSphere }
        };

        public enum Types
        {
            Sphere,
            Box
        }

        /// <summary> Used by Editor script to build primitives</summary>
        public static Primitive ConstructPrimitive(Types primitiveType, List<float> parameters)
        {
            return PrimitiveBuildMap[primitiveType](parameters);
        }

        private static Sphere BuildSphere(List<float> parameters)
        {
            float radius = (parameters.Count != 1 || parameters[0] <= 0) ? 1 : parameters[0];
            return new Sphere(radius);
        }

        private static Box BuildBox(List<float> parameters)
        {
            float width = (parameters.Count < 1 || parameters[0] <= 0) ? 1 : parameters[0];
            float height = (parameters.Count < 2 || parameters[1] <= 0) ? 1 : parameters[1];
            float depth = (parameters.Count < 3 || parameters[2] <= 0) ? 1 : parameters[2];
            return new Box(width, height, depth);
        }

        public abstract IntPtr Generate();
    }
}
