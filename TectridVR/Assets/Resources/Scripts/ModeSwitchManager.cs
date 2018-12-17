using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent(typeof(SculptManager))]
[RequireComponent(typeof(SpawnManager))]
[RequireComponent(typeof(TeleportManager))]
public class ModeSwitchManager : MonoBehaviour
{
    public enum SelectionType
    {
        Object,
        Hierarchy
    }

    public static SelectionType selectionType;

    public enum Modes
    {
        Sculpt,
        Spawn,
        Teleport,
        Boolean
    }

    private Modes _Mode;
    public Modes Mode
    {
        get { return _Mode; }
        set
        {
            _Mode = value;
            foreach (Modes mode in Enum.GetValues(typeof(Modes)))
            {
                modeMap[mode].Activate(mode == _Mode);
            }
        }
    }

    private Dictionary<Modes, ModeManager> modeMap = new Dictionary<Modes, ModeManager>();

    void Start ()
    {
        modeMap.Add(Modes.Sculpt, transform.GetComponent<SculptManager>());
        modeMap.Add(Modes.Spawn, transform.GetComponent<SpawnManager>());
        modeMap.Add(Modes.Teleport, transform.GetComponent<TeleportManager>());
        modeMap.Add(Modes.Boolean, transform.GetComponent<BooleanManager>());
        Mode = Modes.Sculpt;	
	}

    public static void SwitchSelectionType()
    {
        selectionType = selectionType == SelectionType.Object ? SelectionType.Hierarchy : SelectionType.Object;
    }
}
