using System.Collections.Generic;
using System.Linq;
using UnityEngine;

namespace Tectrid
{
    public class SceneManager
    {
        public List<EditableMesh> EditableMeshes { get; private set; }

        private static object syncRoot = new object();
        private static SceneManager instance;

        private List<EditableMesh> _undoRedoList = new List<EditableMesh>();
        private int _undoRedoListIndex = 0;
        private const int _maxUndoRedo = 10;


        private SceneManager()
        {
            EditableMeshes = new List<EditableMesh>();
        }

        /// <summary> Automatically called by any EditableMesh when created </summary>
        public void RegisterEditableMesh(EditableMesh editableMesh)
        {
            if (!EditableMeshes.Contains(editableMesh))
            {
                EditableMeshes.Add(editableMesh);
            }
        }

        /// <summary> Automatically called by any EditableMesh when destroyed </summary>
        public void UnregisterEditableMesh(EditableMesh editableMesh)
        {
            if (EditableMeshes.Contains(editableMesh))
            {
                EditableMeshes.Remove(editableMesh);
            }
        }

        public static SceneManager Instance
        {
            get
            {          
                if (instance == null)
                {
                    lock (syncRoot)
                    {
                        if (instance == null)
                        {
                            instance = new SceneManager();
                        }
                    }
                }
                return instance;
            }
        }

        public EditableMesh GetClosestMesh(Ray ray)
        {
            EditableMesh closestMesh = null;
            float closestDistance = -1;
            Vector3 intersectionPoint = Vector3.zero;
            for (int i = 0; i < EditableMeshes.Count; i++)
            {
                if (EditableMeshes[i].GetClosestIntersectionPoint(ray, ref intersectionPoint))
                {
                    float currDistance = Vector3.Distance(ray.origin, intersectionPoint);
                    if (currDistance < closestDistance || closestDistance == -1)
                    {
                        if (EditableMeshes[i].gameObject.activeSelf == true)
                        {
                            closestDistance = currDistance;
                            closestMesh = EditableMeshes[i];
                        }
                    }
                }
            }
            return closestMesh;
        }

        public EditableMesh GetClosestMesh(Ray ray, ref Vector3 intersectionPoint)
        {
            EditableMesh closestMesh = null;
            float closestDistance = -1;
            for (int i = 0; i < EditableMeshes.Count; i++)
            {
                if (EditableMeshes[i].GetClosestIntersectionPoint(ray, ref intersectionPoint))
                {
                    float currDistance = Vector3.Distance(ray.origin, intersectionPoint);
                    if (currDistance < closestDistance || closestDistance == -1)
                    {
                        if (EditableMeshes[i].gameObject.activeSelf == true)
                        {
                            closestDistance = currDistance;
                            closestMesh = EditableMeshes[i];
                        }
                    }
                }
            }
            return closestMesh;
        }

        #region Undo / Redo

        public void AddToUndoRedoList(EditableMesh editableMesh)
        {
            while (_undoRedoListIndex + 1 < _undoRedoList.Count)
                _undoRedoList.RemoveAt(_undoRedoListIndex);
            if (_undoRedoList.Count == _maxUndoRedo)
            {
                _undoRedoList.RemoveAt(0);
                _undoRedoListIndex--;
            }
            _undoRedoList.Add(editableMesh);
            _undoRedoListIndex = _undoRedoList.Count - 1;
        }

        public void Undo()
        {
            if (_undoRedoList.Count > 0)
            {
                _undoRedoList[_undoRedoListIndex].Undo();
                if (_undoRedoListIndex > 0)
                    _undoRedoListIndex--;
            }
        }

        public void Redo()
        {
            if (_undoRedoList.Count > 0)
            {
                if (_undoRedoListIndex + 1 < _undoRedoList.Count)
                    _undoRedoListIndex++;
                _undoRedoList[_undoRedoListIndex].Redo();
            }
        }

#endregion // ! Undo / Redo
    }
}
