using UnityEngine;
using System;
using System.Runtime.InteropServices;
using Tectrid;

namespace Tectrid.Examples
{
    public class BooleanExample : MonoBehaviour
    {
        public GameObject Object1;
        public GameObject Object2;
        private Mesh _inputMesh2;
        private EditableMesh _inputTectridMesh1;
        private EditableMesh _inputTectridMesh2;

        void Start()
        {
            _inputMesh2 = Object2.GetComponent<MeshFilter>().mesh;
            if (_inputMesh2 != null)
            {
                _inputTectridMesh1 = Object1.AddComponent<EditableMesh>();
                _inputTectridMesh2 = Object2.AddComponent<EditableMesh>();
                _inputTectridMesh1.Build();
                _inputTectridMesh2.Build();
                Object2.GetComponent<MeshRenderer>().enabled = false;
            }
        }

        void Update()
        {
            _inputTectridMesh1.Subtract(_inputTectridMesh2);
            _inputTectridMesh1.Undo(false);
        }
    }
}

