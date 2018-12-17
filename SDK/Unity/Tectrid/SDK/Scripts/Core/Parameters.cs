using UnityEngine;
using System;
using System.Runtime.InteropServices;


namespace Tectrid
{
    public static class Parameters
    {
        public static bool IsMirrorModeActivated()
        {
            return DLL.SculptEngine_IsMirrorModeActivated();
        }

        public static void SetMirrorMode(bool value)
        {
            DLL.SculptEngine_SetMirrorMode(value);
        }
    }
}
