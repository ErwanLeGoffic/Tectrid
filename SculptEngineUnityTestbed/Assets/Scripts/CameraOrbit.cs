using UnityEngine;
using System.Collections;

public class CameraOrbit: MonoBehaviour
{
	public GameObject m_target;
    private float m_horizontalDistanceFromtarget = 5.0f;
	private float m_heightFromtarget = 1.0f;
	public float m_horizontalRotationSpeed = 180.0f;  // In degrees per second
	public float m_verticalRotationSpeed = 180.0f;  // In degrees per second
	Vector2 m_pitchLimits = new Vector2(-85f, 85f);
	private float m_distFromWall = 0.3f;
	public bool m_moveCamera = false;
    public Vector2 m_cameraDpl = Vector2.zero;
    private bool m_btargetBoundsSetFromOutside = false;
    private Bounds m_targetBounds = new Bounds();
 
    void RecurseComputeTargetBounds(GameObject obj)
    {
        Renderer renderer = (Renderer)obj.GetComponent(typeof(Renderer));
        if(renderer != null)
            m_targetBounds.Encapsulate(renderer.bounds);
        for(int i = 0; i < obj.transform.childCount; ++i)
        {
            Transform childTransform = obj.transform.GetChild(i);
            RecurseComputeTargetBounds(childTransform.gameObject);
        }
    }

    void Start()
	{
		m_moveCamera = true;
    }

    void RecomputTargetting()
    {
        // Adjust camera distance from object regarding its size
        if(!m_btargetBoundsSetFromOutside)
            RecurseComputeTargetBounds(m_target);
        m_heightFromtarget = 0.0f;
        m_horizontalDistanceFromtarget = Mathf.Max(Mathf.Max(m_targetBounds.size.x, m_targetBounds.size.y), m_targetBounds.size.z);
        m_horizontalDistanceFromtarget *= 1.2f;
    }

    public void SetTargetBounds(Bounds bounds)
    {
        if(bounds.size != Vector3.zero)
        {
            m_targetBounds = bounds;
            m_btargetBoundsSetFromOutside = true;
        }
    }

    void Update()
    {
        if(!m_moveCamera)
			return;
        RecomputTargetting();
        // Get delta pitch and yaw
        float deltaPitch = m_cameraDpl.y * m_verticalRotationSpeed * Time.fixedDeltaTime;
		float deltaYaw = -m_cameraDpl.x * m_horizontalRotationSpeed * Time.fixedDeltaTime;
		//deltaPitch += Input.GetAxis("CamJoystickY") * m_verticalRotationSpeed * Time.deltaTime;
		//deltaYaw += -Input.GetAxis("CamJoystickX") * m_horizontalRotationSpeed * Time.deltaTime;
		// Clamp pitch to min max boundaries
		float curPitch = transform.eulerAngles.x;
		if(curPitch >= 180.0f)
		{
			curPitch -= 360.0f;
		}
		if(curPitch + deltaPitch > m_pitchLimits.y)
		{
			deltaPitch = m_pitchLimits.y - curPitch;
		}
		if(curPitch + deltaPitch < m_pitchLimits.x)
		{
			deltaPitch = m_pitchLimits.x - curPitch;
		}
        // Compute camera target
        Vector3 camTarget = m_targetBounds.center;
		camTarget.y += m_heightFromtarget;
        // Compute camera position
        Vector3 deltadVect = (transform.position - camTarget).normalized;
		deltadVect *= m_horizontalDistanceFromtarget;
		Quaternion rotationYaw = Quaternion.Euler(0.0f, deltaYaw, 0.0f);
		Quaternion rotationPitch = Quaternion.AngleAxis(deltaPitch, transform.right);
		deltadVect = rotationPitch * rotationYaw * deltadVect;
		transform.position = camTarget + deltadVect;
		// Handle Collision
		Vector3 deltaTargetToPos = transform.position - camTarget;
		int layerMaskExcludeTarget = ~(1 << m_target.layer);    // Prevent to hit ourselves
		Ray castRay = new Ray(camTarget, Vector3.Normalize(deltaTargetToPos));  // From target to viewpoint
		RaycastHit hitInfo;
		if(Physics.SphereCast(castRay, m_distFromWall, out hitInfo, deltaTargetToPos.magnitude, layerMaskExcludeTarget)) // Use a sphere cast to prevent getting into the walls when the camera hit the wall from its "side" (could have done the same with multiple raycasts)
		{
			transform.position = hitInfo.point + (hitInfo.normal * m_distFromWall); // Use the normal to move the camera a bit out of the walls
		}
		// Update camera transform regarding target
		transform.LookAt(camTarget);
	}

    public void ForceRetarget()
    {
        bool saveModeCameraState = m_moveCamera;
        m_moveCamera = true;
        Update();
        m_moveCamera = saveModeCameraState;
    }
}
