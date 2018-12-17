using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ViveControllerInputHandler : MonoBehaviour
{
    private SteamVR_TrackedController _controller;

    private void Start()
    {
        _controller = transform.GetComponent<SteamVR_TrackedController>();
    }

    private void Update()
    {
        PadTouchPosition = new Vector2(_controller.controllerState.rAxis0.x, _controller.controllerState.rAxis0.y);
        PadTouchAngleRad = Mathf.Atan2(PadTouchPosition.y, PadTouchPosition.x);
        PadTouchAngleDeg = PadTouchAngleRad * Mathf.Rad2Deg;

        TriggerUp = (TriggerDown || TriggerStay) && !_controller.triggerPressed;
        TriggerDown = _controller.triggerPressed && !TriggerDown && !TriggerStay;
        TriggerStay = _controller.triggerPressed && !TriggerDown;

        GripUp = (GripDown || GripStay) && !_controller.gripped;
        GripDown = _controller.gripped && !GripDown && !GripStay;
        GripStay = _controller.gripped && !GripDown;

        PadUp = (PadDown || PadStay) && !_controller.padPressed;
        PadDown = _controller.padPressed && !PadDown && !PadStay;
        PadStay = _controller.padPressed && !PadDown;

        PadTouchUp = (PadTouchDown || PadTouchStay) && !_controller.padTouched;
        PadTouchDown = _controller.padTouched && !PadTouchDown && !PadTouchStay;
        PadTouchStay = _controller.padTouched && !PadTouchDown;

        SteamPressed = _controller.steamPressed;
        MenuPressed = _controller.menuPressed;
    }

    #region Inputs 

    public event Action OnTriggerDown;
    private bool _TriggerDown;
    public bool TriggerDown
    {
        get { return _TriggerDown; }
        private set
        {
            _TriggerDown = value;
            if (_TriggerDown)
            {
                if (OnTriggerDown != null)
                    OnTriggerDown();
            }
        }
    }

    public event Action OnTriggerStay;
    private bool _TriggerStay;
    public bool TriggerStay
    {
        get { return _TriggerStay; }
        private set
        {
            _TriggerStay = value;
            if (_TriggerStay)
            {
                if (OnTriggerStay != null)
                    OnTriggerStay();
            }
        }
    }

    public event Action OnTriggerUp;
    private bool _TriggerUp;
    public bool TriggerUp
    {
        get { return _TriggerUp; }
        private set
        {
            _TriggerUp = value;
            if (_TriggerUp)
            {
                if (OnTriggerUp != null)
                    OnTriggerUp();
            }
        }
    }

    public event Action OnGripDown;
    private bool _GripDown;
    public bool GripDown
    {
        get { return _GripDown; }
        private set
        {
            _GripDown = value;
            if (_GripDown)
            {
                if (OnGripDown != null)
                    OnGripDown();
            }
        }
    }

    public event Action OnGripStay;
    private bool _GripStay;
    public bool GripStay
    {
        get { return _GripStay; }
        private set
        {
            _GripStay = value;
            if (_GripStay)
            {
                if (OnGripStay != null)
                    OnGripStay();
            }
        }
    }

    public event Action OnGripUp;
    private bool _GripUp;
    public bool GripUp
    {
        get { return _GripUp; }
        private set
        {
            _GripUp = value;
            if (_GripUp)
            {
                if (OnGripUp != null)
                    OnGripUp();
            }
        }
    }

    public event Action OnPadDown;
    private bool _PadDown;
    public bool PadDown
    {
        get { return _PadDown; }
        private set
        {
            _PadDown = value;
            if (_PadDown)
            {
                if (OnPadDown != null)
                    OnPadDown();
            }
        }
    }

    public event Action OnPadStay;
    private bool _PadStay;
    public bool PadStay
    {
        get { return _PadStay; }
        private set
        {
            _PadStay = value;
            if (_PadStay)
            {
                if (OnPadStay != null)
                    OnPadStay();
            }
        }
    }

    public event Action OnPadUp;
    private bool _PadUp;
    public bool PadUp
    {
        get { return _PadUp; }
        private set
        {
            _PadUp = value;
            if (_PadUp)
            {
                if (OnPadUp != null)
                    OnPadUp();
            }
        }
    }

    public event Action OnPadTouchDown;
    private bool _PadTouchDown;
    public bool PadTouchDown
    {
        get { return _PadTouchDown; }
        private set
        {
            _PadTouchDown = value;
            if (_PadTouchDown)
            {
                if (OnPadTouchDown != null)
                    OnPadTouchDown();
            }
        }
    }

    public event Action OnPadTouchStay;
    private bool _PadTouchStay;
    public bool PadTouchStay
    {
        get { return _PadTouchStay; }
        private set
        {
            _PadTouchStay = value;
            if (_PadTouchStay)
            {
                if (OnPadTouchStay != null)
                    OnPadTouchStay();
            }
        }
    }

    public event Action OnPadTouchUp;
    private bool _PadTouchUp;
    public bool PadTouchUp
    {
        get { return _PadTouchUp; }
        private set
        {
            _PadTouchUp = value;
            if (_PadTouchUp)
            {
                if (OnPadTouchUp != null)
                    OnPadTouchUp();
            }
        }
    }

    public event Action OnSteamPressed;
    private bool _SteamPressed;
    public bool SteamPressed
    {
        get { return _SteamPressed; }
        private set
        {
            _SteamPressed = value;
            if (_SteamPressed)
            {
                if (OnSteamPressed != null)
                    OnSteamPressed();
            }
        }
    }

    public event Action OnMenuPressed;
    private bool _MenuPressed;
    public bool MenuPressed
    {
        get { return _MenuPressed; }
        private set
        {
            _MenuPressed = value;
            if (_MenuPressed)
            {
                if (OnMenuPressed != null)
                    OnMenuPressed();
            }
        }
    }

    public float PadTouchAngleDeg { get; private set; }
    public float PadTouchAngleRad { get; private set; }
    public Vector2 PadTouchPosition { get; private set; }
    #endregion // ! Inputs
}
