using System.Collections;
using System.Collections.Generic;
using Tectrid;
using UnityEngine;

public class TargetingTool : VRTool
{

    public GameObject Target;

    [SerializeField]
    private LineRenderer _uiTargettingRay;

	// Update is called once per frame
	void Update ()
    {
        Ray ray = new Ray(transform.position, transform.forward);
        RaycastHit hitInfos;

        Vector3 meshIntersectionPoint = Vector3.zero;
        EditableMesh meshHit = SceneManager.Instance.GetClosestMesh(ray, ref meshIntersectionPoint);

        GameObject objectHit = null;
        Vector3 objectHitIntersectionPoint = Vector3.zero;

        Vector3 closestIntersectionPoint = Vector3.zero;

        if (Physics.Raycast(ray, out hitInfos))
        {
            objectHit = hitInfos.transform.gameObject;
            objectHitIntersectionPoint = hitInfos.point;
        }
        if (meshHit != null && objectHit != null)
        {
            if (objectHit != meshHit.gameObject)
            {
                float distanceToMesh = Vector3.Distance(ray.origin, meshIntersectionPoint);
                float distanceToObject = Vector3.Distance(ray.origin, objectHitIntersectionPoint);
                if (distanceToMesh < distanceToObject)
                {
                    Target = meshHit.gameObject;
                    closestIntersectionPoint = meshIntersectionPoint;
                }
                else
                {
                    Target = objectHit;
                    closestIntersectionPoint = objectHitIntersectionPoint;
                }
            }
            else
            {
                Target = meshHit.gameObject;
                closestIntersectionPoint = meshIntersectionPoint;
            }             
        }
        else
        {
            Target = objectHit == null ? meshHit == null ? null : meshHit.gameObject : objectHit;
            closestIntersectionPoint = objectHit == null ? meshHit == null ? Vector3.zero : meshIntersectionPoint : objectHitIntersectionPoint;
        }
        UpdadeTargettingRay(closestIntersectionPoint);
    }

    private void UpdadeTargettingRay(Vector3 intersectionPoint)
    {
        // Update ray
        Ray ray = new Ray(transform.position, transform.forward);
        if (intersectionPoint != Vector3.zero)
        {
            _uiTargettingRay.SetPosition(0, transform.position);
            _uiTargettingRay.SetPosition(1, transform.position + transform.forward * Vector3.Distance(transform.position, intersectionPoint));
        }
        else if (Camera.main != null)
        {
            _uiTargettingRay.SetPosition(0, transform.position);
            _uiTargettingRay.SetPosition(1, transform.position + transform.forward * 2);
        }
    }

    public void ChangeRayColor(Color color)
    {
        _uiTargettingRay.startColor = color;
        _uiTargettingRay.endColor = color;
    }

    public Color GetRayStartColor()
    {
        return _uiTargettingRay.startColor;
    }
}
