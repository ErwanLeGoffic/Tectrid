using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class VRMenu : MonoBehaviour {

    [SerializeField] protected GameObject _player;
    [SerializeField] protected float yOffset = 1.5f;
    [SerializeField] protected float forwardOffset = 2f;

    protected Vector3 _playerPrevPosition;

    protected virtual void OnEnable()
    {
        ResetPosition();
        _playerPrevPosition = _player.transform.position;
    }

    protected virtual void Update()
    {
        transform.position += _player.transform.position - _playerPrevPosition;
        _playerPrevPosition = _player.transform.position;
    }

    public void ResetPosition()
    {
        transform.position = _player.transform.position + _player.transform.forward.normalized * forwardOffset;
        transform.position = new Vector3(transform.position.x, yOffset, transform.position.z);
        transform.rotation = Quaternion.Euler(0, _player.transform.rotation.eulerAngles.y, 0);
    }

    public virtual void Confirm()
    {

    }

    public void Exit()
    {
        gameObject.SetActive(false);
    }
}
