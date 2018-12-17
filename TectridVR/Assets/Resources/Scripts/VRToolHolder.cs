using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

public class VRToolHolder : MonoBehaviour {

    protected List<VRTool> _tools;

    protected virtual void Start()
    {
        _tools = transform.GetComponents<VRTool>().ToList();
	}
	
    public T GetTool<T>() where T : VRTool
    {
       for (int i = 0; i < _tools.Count; i++)
        {
            if (_tools[i].GetType() == typeof(T))
            {
                return _tools[i] as T;
            }
        }
        return null;
    }
}
