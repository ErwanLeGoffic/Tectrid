using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.UI;

public class FileBrowser : VRMenu
{
    [SerializeField] private GameObject _directoryFieldPrefab;
    [SerializeField] private GameObject _fileFieldPrefab;
    [SerializeField] private GameObject _directoryContainer;
    [SerializeField] private GameObject _fileContainer;
    [SerializeField] private Scrollbar _directoriesScrollbar;
    [SerializeField] private Scrollbar _filesScrollbar;
    [SerializeField] private TextMesh _selectionText;
    [SerializeField] private TextMesh _currentDirectoryText;
    [SerializeField] private SpawnTool _spawnTool;

    private string _currentDirectory = null;
    private string CurrentDirectory
    {
        get { return _currentDirectory; }
        set
        {
            _currentDirectory = value;
            _currentDirectoryText.text = _currentDirectory;
        }
    }
    private string _selection;
    private string Selection
    {
        get { return _selection; }
        set
        {
            _selection = value;
        }
    }

    protected override void OnEnable()
    {
        if (_currentDirectory == null)
            CurrentDirectory = Application.dataPath;
        Debug.Log(_currentDirectory);
        UpdateDirectories();
        UpdateFiles();
        base.OnEnable();
    }

    public void EnterDirectory(string name)
    {
        try
        {
            string newCurrentDirectory = CurrentDirectory + (CurrentDirectory != "" ? (CurrentDirectory[CurrentDirectory.Length - 1] != '/' && CurrentDirectory[CurrentDirectory.Length - 1] != '\\' ? "/" : "") : "") + name;
            Directory.GetFiles(newCurrentDirectory);
            CurrentDirectory = newCurrentDirectory;
            UpdateDirectories();
            UpdateFiles();
        }
        catch (UnauthorizedAccessException e)
        {

        }

    }

    public void UpdateSelection(string fileName)
    {
        _selectionText.text = fileName;
        Selection = _currentDirectory + "/" + fileName;
    }

    public void Back()
    {
        DirectoryInfo parentInfos = Directory.GetParent(CurrentDirectory);
        if (parentInfos != null)
        {
                Directory.GetFiles(parentInfos.FullName);
                CurrentDirectory = parentInfos.FullName;
                UpdateDirectories();
                UpdateFiles();
        }
        else
        {
            CurrentDirectory = "";
            UpdateDirectories(Directory.GetLogicalDrives());
            foreach (Transform child in _fileContainer.transform)
            {
                Destroy(child.gameObject);
            }
        }
    }

    public override void Confirm()
    {
        if (_spawnTool.enabled)
            _spawnTool.BuildObjectMesh(Selection);
        gameObject.SetActive(false);
    }

    void UpdateDirectories(string[] directories)
    {
        
        foreach (Transform child in _directoryContainer.transform)
        {
            Destroy(child.gameObject);
        }
        for (int i = 0; i < directories.Length; i++)
        {
            GameObject newDirectoryField = Instantiate(_directoryFieldPrefab, _directoryContainer.transform);
            newDirectoryField.GetComponent<DirectoryButton>().directoryName = directories[i];
            newDirectoryField.GetComponent<DirectoryButton>().fileBrowser = this;
        }
    }

    void UpdateDirectories()
    {
        string[] directories = Directory.GetDirectories(_currentDirectory);
        foreach(Transform child in _directoryContainer.transform)
        {
            Destroy(child.gameObject);
        }
        for (int i = 0; i < directories.Length; i++)
        {
            GameObject newDirectoryField = Instantiate(_directoryFieldPrefab, _directoryContainer.transform);
            newDirectoryField.GetComponent<DirectoryButton>().directoryName = Path.GetFileName(directories[i]);
            newDirectoryField.GetComponent<DirectoryButton>().fileBrowser = this;
        }
    }

    void UpdateFiles()
    {
        string[] files = Directory.GetFiles(_currentDirectory);
        foreach (Transform child in _fileContainer.transform)
        {
            Destroy(child.gameObject);
        }
        for (int i = 0; i < files.Length; i++)
        {
            if (Path.GetExtension(files[i]) == ".obj")
            {
                GameObject newDirectoryField = Instantiate(_fileFieldPrefab, _fileContainer.transform);
                newDirectoryField.GetComponent<FileButton>().fileName = Path.GetFileName(files[i]);
                newDirectoryField.GetComponent<FileButton>().fileBrowser = this;
            }
        }
    }


}
