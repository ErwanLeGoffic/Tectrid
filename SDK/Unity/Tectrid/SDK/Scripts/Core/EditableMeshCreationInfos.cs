using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using Tectrid.Primitives;
using UnityEngine;

namespace Tectrid
{
    [Serializable]
    public class EditableMeshCreationInfos
    {
        public enum CreationType { Empty, ObjectMeshSubstitution, MeshCopy, FromFile, Primitive }

        [SerializeField]
        private CreationType _currentCreationType;

        [SerializeField]
        private Primitive.Types _primitiveType;
        [SerializeField]
        private List<float> _primitiveParameters;

        [SerializeField]
        private string _fileBinaryData;
        [SerializeField]
        private string _filePath;

        [SerializeField]
        private Mesh _meshToCopy;

        [SerializeField]
        private Primitive.Types _previousPrimitiveType;

        public void BuildPreview(EditableMesh editableMesh)
        {
            switch (_currentCreationType)
            {
                case CreationType.FromFile:
                    editableMesh.Build(_fileBinaryData, _filePath);
                    break;
                case CreationType.MeshCopy:
                    editableMesh.Build(_meshToCopy);
                    break;
                case CreationType.ObjectMeshSubstitution:
                    editableMesh.Build();
                    break;
                case CreationType.Primitive:
                    editableMesh.Build(Primitive.ConstructPrimitive(_primitiveType, _primitiveParameters));
                    break;
                case CreationType.Empty:
                    editableMesh.Unbuild();
                    break;
                default:
                    break;
            }
        }
    }
}
