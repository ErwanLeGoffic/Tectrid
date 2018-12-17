using System;
using System.Runtime.InteropServices;

namespace Tectrid
{
    public class Brush
    {
        public Type BrushType { get; private set; }

        public enum Type
        {
            Draw,
            Inflate,
            Flatten,
            Drag,
            Dig,
            CADDrag
        };

        public Brush(EditableMesh associatedTectridMesh, Type brushType)
        {         
            switch (brushType)
            {
                case Type.Draw:
                    _brush = DLL.BrushDraw_Create(associatedTectridMesh.GetInnerPtr());
                    break;
                case Type.Inflate:
                    _brush = DLL.BrushInflate_Create(associatedTectridMesh.GetInnerPtr());
                    break;
                case Type.Flatten:
                    _brush = DLL.BrushFlatten_Create(associatedTectridMesh.GetInnerPtr());
                    break;
                case Type.Drag:
                    _brush = DLL.BrushDrag_Create(associatedTectridMesh.GetInnerPtr());
                    break;
                case Type.Dig:
                    _brush = DLL.BrushDig_Create(associatedTectridMesh.GetInnerPtr());
                    break;
                case Type.CADDrag:
                    _brush = DLL.BrushCADDrag_Create(associatedTectridMesh.GetInnerPtr());
                    break;
            }
            BrushType = brushType;
        }

        ~Brush()
        {
            DLL.Brush_Delete(_brush);
        }

        public void StartStroke()
        {
            DLL.Brush_StartStroke(_brush);
        }

        public void UpdateStroke(UnityEngine.Matrix4x4 matrix, UnityEngine.Vector3 rayOrigin, UnityEngine.Vector3 rayDirection, float rayLength, float radius, float strengthRatio)
        {
            float[] rotAndScale3x3Matrix = { matrix.m00, matrix.m01, matrix.m02, matrix.m10, matrix.m11, matrix.m12, matrix.m20, matrix.m21, matrix.m22 };
            float[] position = { matrix.m03, matrix.m13, matrix.m23 };
            GCHandle gcRotAndScale3x3Matrix = GCHandle.Alloc(rotAndScale3x3Matrix, GCHandleType.Pinned);
            GCHandle gcPosition = GCHandle.Alloc(position, GCHandleType.Pinned);
            GCHandle gcRayPos = GCHandle.Alloc(rayOrigin, GCHandleType.Pinned);
            GCHandle gcRayDir = GCHandle.Alloc(rayDirection, GCHandleType.Pinned);
            DLL.Brush_UpdateStroke(_brush, gcRotAndScale3x3Matrix.AddrOfPinnedObject(), gcPosition.AddrOfPinnedObject(), gcRayPos.AddrOfPinnedObject(), gcRayDir.AddrOfPinnedObject(), rayLength, radius, strengthRatio);
            gcRayPos.Free();
            gcRayDir.Free();
            gcRotAndScale3x3Matrix.Free();
            gcPosition.Free();          
        }

        public void EndStroke()
        {
            DLL.Brush_EndStroke(_brush);
        }

        private IntPtr _brush;
    }
}
