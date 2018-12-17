using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PrimitiveMeshPicker : VRUIButton
{
    public enum PrimitiveType { UnityMesh, Sphere, Box }

    [SerializeField] private PrimitiveType _primitiveType;
    private Transform _correspondingPrimitive;
    private MeshFilter _primitiveMeshFilter;
    
	// Use this for initialization
	protected override void Start ()
    {
        _correspondingPrimitive = transform.GetChild(0);
        _renderer = _correspondingPrimitive.GetComponent<MeshRenderer>();
        _primitiveMeshFilter = _correspondingPrimitive.GetComponent<MeshFilter>();
	}

    public override void Select(GameObject sender)
    {
        SpawnTool _spawnerTool = sender.GetComponent<SpawnTool>();
        if (_spawnerTool != null)
        {
            if (_primitiveType == PrimitiveType.UnityMesh)
            {
                _spawnerTool.BuildObjectMesh(_primitiveMeshFilter.mesh);
            }
            else if (_primitiveType == PrimitiveType.Box)
                _spawnerTool.BuildObjectMesh(Tectrid.Primitives.Primitive.Types.Box);
            else if (_primitiveType == PrimitiveType.Sphere)
                _spawnerTool.BuildObjectMesh(Tectrid.Primitives.Primitive.Types.Sphere);
        }
    }
}
