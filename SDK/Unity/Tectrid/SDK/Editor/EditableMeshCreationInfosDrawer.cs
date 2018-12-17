using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using Tectrid.Primitives;
using UnityEditor;
using UnityEngine;


namespace Tectrid.EditorExtensions
{


    // Use this for initialization
    [CustomPropertyDrawer(typeof(EditableMeshCreationInfos))]
    public class EditableMeshCreationInfosDrawer : PropertyDrawer
    {
        // Helps compute field positions in inspector
        private const float _propertyHeight = 16;
        private const float _indentSize = 8;
        private int _propertyCount;

        private EditableMesh _editableMesh = null;
        private EditableMeshCreationInfos _creationInfos = null;

        // Properties the user will change, mapped to correspond EditableMeshCreationInfos instance properties
        private SerializedProperty _previousPrimitiveType;
        private SerializedProperty _creationType;
        private SerializedProperty _filePath;
        private SerializedProperty _binaryData;
        private SerializedProperty _meshProperty;
        private SerializedProperty _primitiveType;
        private SerializedProperty _primitiveParameters;

        private delegate void DrawCorrespondingFieldsDelegate(Rect startPosition);
        private delegate void DrawCorrespondingPrimitiveFieldsDelegate(Rect position);

        private Dictionary<EditableMeshCreationInfos.CreationType, DrawCorrespondingFieldsDelegate> FieldDrawersMap;
        private Dictionary<Primitive.Types, DrawCorrespondingPrimitiveFieldsDelegate> PrimitiveFieldDrawersMap;

        private void InitProperties(SerializedProperty creationInfos)
        {
            _creationType = creationInfos.FindPropertyRelative("_currentCreationType");
            _filePath = creationInfos.FindPropertyRelative("_filePath");
            _binaryData = creationInfos.FindPropertyRelative("_fileBinaryData");
            _meshProperty = creationInfos.FindPropertyRelative("_meshToCopy");
            _primitiveType = creationInfos.FindPropertyRelative("_primitiveType");
            _primitiveParameters = creationInfos.FindPropertyRelative("_primitiveParameters");
            _previousPrimitiveType = creationInfos.FindPropertyRelative("_previousPrimitiveType");

            FieldDrawersMap = new Dictionary<EditableMeshCreationInfos.CreationType, DrawCorrespondingFieldsDelegate>()
            {
                {EditableMeshCreationInfos.CreationType.FromFile, DrawFilePathField },
                {EditableMeshCreationInfos.CreationType.MeshCopy, DrawMeshCopyField },
                {EditableMeshCreationInfos.CreationType.Primitive, DrawPrimitiveEnumChoice },
                {EditableMeshCreationInfos.CreationType.Empty, null },
                {EditableMeshCreationInfos.CreationType.ObjectMeshSubstitution, null }
            };

            PrimitiveFieldDrawersMap = new Dictionary<Primitive.Types, DrawCorrespondingPrimitiveFieldsDelegate>()
            {
                {Primitive.Types.Sphere, DrawSphereFields },
                {Primitive.Types.Box, DrawBoxFields }
            };
        }

        public override void OnGUI(Rect startPosition, SerializedProperty creationInfos, GUIContent label)
        {
            if (_editableMesh == null)
            {
                object editableMesh = GetParent(creationInfos);
                _editableMesh = editableMesh as EditableMesh;
            }
            if (_creationInfos == null)
            {
                var obj = fieldInfo.GetValue(creationInfos.serializedObject.targetObject);
                _creationInfos = obj as EditableMeshCreationInfos;
                if (obj.GetType().IsArray)
                {
                    var index = Convert.ToInt32(new string(creationInfos.propertyPath.Where(c => char.IsDigit(c)).ToArray()));
                    _creationInfos = ((EditableMeshCreationInfos[])obj)[index];
                }
                InitProperties(creationInfos);
            }

            _propertyCount = 0;
            GUI.Label(startPosition, "Mesh Creation Informations");
            DrawCreationTypeEnumChoice(startPosition);
            if (FieldDrawersMap[(EditableMeshCreationInfos.CreationType)_creationType.enumValueIndex] != null)
                FieldDrawersMap[(EditableMeshCreationInfos.CreationType)_creationType.enumValueIndex](startPosition);
            DrawPreviewButton(startPosition);
        }

        public override float GetPropertyHeight(SerializedProperty property, GUIContent label)
        {
            return base.GetPropertyHeight(property, label) + (_propertyHeight * _propertyCount);
        }

        private void DrawPreviewButton(Rect startPosition)
        {
            _propertyCount++;
            Rect filePathFieldRect = new Rect(startPosition.x + startPosition.width / 2, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            if (GUI.Button(filePathFieldRect, "Draw Preview"))
            {
                _creationInfos.BuildPreview(_editableMesh);
            }
        }

        private void DrawCreationTypeEnumChoice(Rect startPosition)
        {
            _propertyCount++;
            Rect creationTypePrefixLabelPosition = new Rect(startPosition.x + _indentSize, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            EditorGUI.PrefixLabel(creationTypePrefixLabelPosition, new GUIContent("Creation Type"));
            Rect creationTypeRect = new Rect(startPosition.x + startPosition.width / 2, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            EditorGUI.PropertyField(creationTypeRect, _creationType, GUIContent.none);
        }

        private void DrawFilePathField(Rect startPosition)
        {
            _propertyCount++;
            Rect prefixLabelRect = new Rect(startPosition.x + _indentSize * 2, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            EditorGUI.PrefixLabel(prefixLabelRect, new GUIContent("File Path"));
            Rect filePathFieldRect = new Rect(startPosition.x + startPosition.width / 2, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            if (GUI.Button(filePathFieldRect, Path.GetFileNameWithoutExtension(_filePath.stringValue)))
            {
                string filePath = EditorUtility.OpenFilePanel("Select Mesh", Application.dataPath, "obj");
                if (_filePath.stringValue != filePath)
                {
                    _filePath.stringValue = filePath;
                    if (_filePath.stringValue != string.Empty)
                    {
                        using (StreamReader reader = new StreamReader(_filePath.stringValue))
                        {
                            _binaryData.stringValue = reader.ReadToEnd();
                        }
                    }
                }
            }
        }

        private void DrawMeshCopyField(Rect startPosition)
        {
            _propertyCount++;
            Rect prefixLabelRect = new Rect(startPosition.x + _indentSize * 2, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            EditorGUI.PrefixLabel(prefixLabelRect, new GUIContent("Mesh To Copy"));
            Rect meshFieldRect = new Rect(startPosition.x + startPosition.width / 2, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            EditorGUI.ObjectField(meshFieldRect, _meshProperty, GUIContent.none);
        }

        private void DrawPrimitiveEnumChoice(Rect startPosition)
        {
            _propertyCount++;
            Rect prefixLabelRect = new Rect(startPosition.x + _indentSize * 2, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            EditorGUI.PrefixLabel(prefixLabelRect, new GUIContent("Mesh To Copy"));
            Rect enumChoiceRect = new Rect(startPosition.x + startPosition.width / 2, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            EditorGUI.PropertyField(enumChoiceRect, _primitiveType, GUIContent.none);
            DrawCurrentPrimitiveInformations(startPosition);
        }

        private void DrawCurrentPrimitiveInformations(Rect startPosition)
        {
            Rect position = new Rect(startPosition.x + _indentSize * 3, startPosition.y, startPosition.width, startPosition.height);
            PrimitiveFieldDrawersMap[(Primitive.Types)_primitiveType.enumValueIndex](position);
        }

        private float GetFloatFieldValue(Rect startPosition, string labelName, float prevValue)
        {
            _propertyCount++;
            Rect prefixLabelRect = new Rect(startPosition.x + _indentSize, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            EditorGUI.PrefixLabel(prefixLabelRect, new GUIContent(labelName));
            Rect floatFieldRect = new Rect(startPosition.x + startPosition.width / 2, startPosition.y + _propertyHeight * _propertyCount, startPosition.width / 2, _propertyHeight);
            return EditorGUI.FloatField(floatFieldRect, prevValue);
        }

        #region Primitive Field Drawing

        private void DrawSphereFields(Rect position)
        {
            if ((Primitive.Types)_previousPrimitiveType.enumValueIndex != Primitive.Types.Sphere)
            {
                _primitiveParameters.ClearArray();
                _previousPrimitiveType.enumValueIndex = (int)Primitive.Types.Sphere;
            }
            if (_primitiveParameters.arraySize != 1)
            {
                _primitiveParameters.arraySize = 1;
                _primitiveParameters.GetArrayElementAtIndex(0).floatValue = 0;
            }
            float sphereRadius = GetFloatFieldValue(position, "Radius", _primitiveParameters.GetArrayElementAtIndex(0).floatValue);
            _primitiveParameters.GetArrayElementAtIndex(0).floatValue = sphereRadius;
        }

        private void DrawBoxFields(Rect position)
        {
            if ((Primitive.Types)_previousPrimitiveType.enumValueIndex != Primitive.Types.Box)
            {
                _primitiveParameters.ClearArray();
                _previousPrimitiveType.enumValueIndex = (int)Primitive.Types.Box;
            }
            if (_primitiveParameters.arraySize != 3)
            {
                _primitiveParameters.arraySize = 3;
                _primitiveParameters.GetArrayElementAtIndex(0).floatValue = 0;
                _primitiveParameters.GetArrayElementAtIndex(1).floatValue = 0;
                _primitiveParameters.GetArrayElementAtIndex(2).floatValue = 0;
            }
            float boxWidth = GetFloatFieldValue(position, "Width", _primitiveParameters.GetArrayElementAtIndex(0).floatValue);
            _primitiveParameters.GetArrayElementAtIndex(0).floatValue = boxWidth;
            float boxHeight = GetFloatFieldValue(position, "Height", _primitiveParameters.GetArrayElementAtIndex(1).floatValue);
            _primitiveParameters.GetArrayElementAtIndex(1).floatValue = boxHeight;
            float boxDepth = GetFloatFieldValue(position, "Depth", _primitiveParameters.GetArrayElementAtIndex(2).floatValue);
            _primitiveParameters.GetArrayElementAtIndex(2).floatValue = boxDepth;
        }

        #endregion // ! Primitive Field Drawing

        #region Reflection to get gameobject to the owner of the EditableMeshCreationInfos field

        public object GetParent(SerializedProperty prop)
        {
            var path = prop.propertyPath.Replace(".Array.data[", "[");
            object obj = prop.serializedObject.targetObject;
            var elements = path.Split('.');
            foreach (var element in elements.Take(elements.Length - 1))
            {
                if (element.Contains("["))
                {
                    var elementName = element.Substring(0, element.IndexOf("["));
                    var index = Convert.ToInt32(element.Substring(element.IndexOf("[")).Replace("[", "").Replace("]", ""));
                    obj = GetValue(obj, elementName, index);
                }
                else
                {
                    obj = GetValue(obj, element);
                }
            }
            return obj;
        }

        public object GetValue(object source, string name)
        {
            if (source == null)
                return null;
            var type = source.GetType();
            var f = type.GetField(name, BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance);
            if (f == null)
            {
                var p = type.GetProperty(name, BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.IgnoreCase);
                if (p == null)
                    return null;
                return p.GetValue(source, null);
            }
            return f.GetValue(source);
        }

        public object GetValue(object source, string name, int index)
        {
            var enumerable = GetValue(source, name) as IEnumerable;
            var enm = enumerable.GetEnumerator();
            while (index-- >= 0)
                enm.MoveNext();
            return enm.Current;
        }

        #endregion // ! Dirty reflection
    }
}
