using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Tectrid.Primitives
{
    public class InternalPrimitiveFactory
    {
        private delegate IntPtr BuildPrimitiveDelegate(List<float> parameters);

        private static readonly Dictionary<Primitive.Types, BuildPrimitiveDelegate> PrimitiveBuildMap = new Dictionary<Primitive.Types, BuildPrimitiveDelegate>
        {
            {Primitive.Types.Box, BuildBox },
            {Primitive.Types.Sphere, BuildSphere }
        };

        public static IntPtr BuildInternalPrimitive(Primitive.Types primitiveType, List<float> parameters)
        {
            return PrimitiveBuildMap[primitiveType](parameters);
        }

        private static IntPtr BuildBox(List<float> parameters)
        {
            float width = (parameters.Count < 1 || parameters[0] <= 0) ? 1 : parameters[0];
            float height = (parameters.Count < 2 || parameters[1] <= 0) ? 1 : parameters[1];
            float depth = (parameters.Count < 3 || parameters[2] <= 0) ? 1 : parameters[2];
            return DLL.GenBox_Generate(width, height, depth);
        }

        private static IntPtr BuildSphere(List<float> parameters)
        {
            float radius = (parameters.Count != 1 || parameters[0] <= 0) ? 1 : parameters[0];
            return DLL.GenSphere_Generate(radius);
        }
    }
}
