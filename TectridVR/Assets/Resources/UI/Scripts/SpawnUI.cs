using System.Collections;
using System.Collections.Generic;
using UnityEngine;

class SpawnUI : MonoBehaviour
{
    [SerializeField] private GameObject _shelfPivot;
    [SerializeField] private GameObject _primitives;
    [SerializeField] private float _shelfAnimationDuration;
    [SerializeField] private Vector3 _shelfTargetScale;

    private Coroutine _animation = null;
    private Vector3 _shelfStartScale;

    private void OnEnable()
    {
        _shelfStartScale = _shelfPivot.transform.localScale;
        _animation = StartCoroutine("PlaySpawnUIAnimation");
    }

    private void OnDisable()
    {
        if (_animation != null)
            StopCoroutine(_animation);
        _shelfPivot.transform.localScale = _shelfStartScale;
        _primitives.SetActive(false);
    }

    IEnumerator PlaySpawnUIAnimation()
    {
        float timer = 0;
        while (timer < _shelfAnimationDuration)
        {
            _shelfPivot.transform.localScale = Vector3.Lerp(_shelfStartScale, _shelfTargetScale, timer / _shelfAnimationDuration);
            timer += Time.deltaTime;
            yield return null;
        }
        _primitives.SetActive(true);
    }
}
