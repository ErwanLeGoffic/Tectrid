/// <reference path="Manipulator.ts"/>
/// <reference path="Profiler.ts"/>
/// <reference path="SubMesh.ts"/>

enum BrushType
{
    Draw,
    Inflate,
    Flatten,
    Drag,
    Dig
}

enum CsgType
{
    Merge,
    Subtract,
    Intersect
}

// For FileSaver.js
declare function saveAs(blob: Blob, path: string);

// For Astroprint
declare module astroprint
{
    function importDesignByBlob(blob: Blob, content_type: string, name: string);
}

// Get URL parameters
function getQueryStringValue(key)
{
    return decodeURIComponent(window.location.search.replace(new RegExp("^(?:.*[&\\?]" + encodeURIComponent(key).replace("/[\.\+\*]/g", "\\$&") + "(?:\\=([^&]*))?)?.*$", "i"), "$1"));
}

// App class
class AppMain
{
    private _canvas: HTMLCanvasElement;
    private _engine: BABYLON.Engine;
    private _scene: BABYLON.Scene;
    private _camera: BABYLON.ArcRotateCamera;
    private _light: BABYLON.PointLight;
    private _meshItem: Module.Mesh;
    private _meshItemToCreateOrCombine: Module.Mesh;
    private _isInCombineMode: boolean = false;
    private _booleanOn: boolean = true;
    private _mesh: BABYLON.Mesh;
    private _meshToCreateOrCombine: BABYLON.Mesh;
    private _manipulator: Manipulator;
    private _uiRingCursor: BABYLON.LinesMesh;
    private _brushDraw: Module.BrushDraw;
    private _brushInflate: Module.BrushInflate;
    private _brushFlatten: Module.BrushFlatten;
    private _brushDrag: Module.BrushDrag;
    private _brushDig: Module.BrushDig;
    private _profiler: Profiler;
    private _sculptPoint: BABYLON.Vector2;
    private _lastSculptPoint: BABYLON.Vector2;
    private _uiCursorScreenPos: BABYLON.Vector2 = BABYLON.Vector2.Zero();
    private _sculpting: boolean = false;
    private _brushType: BrushType = BrushType.Draw;
    private _revert: boolean = false;
    private _modelRadius: number = 1;
    private _sculptingRadius: number = 0.12;
    private _sculptingStrengthRatio: number = 0.5;
    private _uiSculptingSize: number = 12;
    private _DEBUG_BoundingBoxRenderer: BABYLON.BoundingBoxRenderer;
    private _fullScreen: boolean = false;
    private _rayLength: number = 3.402823466e+38;
    private _material: BABYLON.StandardMaterial;
    private _useSubMeshes: boolean = false;
    private _babylonSubMeshes = {}; // map submesh ID to submesh

    constructor(canvasElement: string)
    {
        if(Module.SculptEngine.HasExpired())
            alert("Product has expired on " + Module.SculptEngine.GetExpirationDate() + "\nIt won't function anymore, please update your version.");
        // Create canvas and engine
        this._canvas = <HTMLCanvasElement>document.getElementById(canvasElement);
        this._engine = new BABYLON.Engine(this._canvas, true);
        this._engine.switchFullscreen
        // Link to UI
        // Top Left
        document.getElementById("fullScreenButton").onclick = function ()
        {
            if(!(<any>window).app._fullScreen)
            {
                (<any>window).app._fullScreen = true;
                (<any>window).app.launchIntoFullscreen(document.documentElement);
            }
            else
            {
                (<any>window).app._fullScreen = false;
                (<any>window).app.exitFullscreen();
            }
        };
        let profilerCheckBox: HTMLInputElement = <HTMLInputElement>document.getElementById('profilerCheckBox');
        if(profilerCheckBox != null)
        {
            profilerCheckBox.onchange = function ()
            {
                if(profilerCheckBox.checked)
                {
                    // Create profiler if not created yet
                    if((<any>window).app._profiler == null)
                    {
                        (<any>window).app._profiler = new Profiler((<any>window).app._engine);
                        (<any>window).app._profiler.Start();
                        (<any>window).app._scene.registerAfterRender(() =>
                        {
                            (<any>window).app._profiler.Update();
                        });
                    }
                    (<any>window).app._profiler.Start();
                }
                else
                {
                    (<any>window).app._profiler.Stop();
                }
            };
        }
        //(<any>document.getElementById("colorSelector")).colorselector = new colorselector();
        // Top right
        document.getElementById("drawButton").onclick = function () { (<any>window).app.selectBrushType(BrushType.Draw, this); };
        //document.getElementById("inflateButton").onclick = function () { (<any>window).app.selectBrushType(BrushType.Inflate, this); };
        document.getElementById("digButton").onclick = function () { (<any>window).app.selectBrushType(BrushType.Dig, this); };
        document.getElementById("flattenButton").onclick = function () { (<any>window).app.selectBrushType(BrushType.Flatten, this); };
        document.getElementById("dragButton").onclick = function () { (<any>window).app.selectBrushType(BrushType.Drag, this); };
        let mirrorSwitch: HTMLInputElement = <HTMLInputElement>document.getElementById('mirrorSwitch');
        if(mirrorSwitch != null)
        {
            mirrorSwitch.onchange = function ()
            {
                Module.SculptEngine.SetMirrorMode(mirrorSwitch.checked);
            };
        }
        // Bottom right
        let sliderStrength = new Slider('#sliderStrength', {
            formatter: function (value)
            {
                if((<any>window).app != null)
                    (<any>window).app._sculptingStrengthRatio = value;
                return 'Current value: ' + value;
            }
        });
        let sliderSize = new Slider('#sliderSize', {
            formatter: function (value)
            {
                if((<any>window).app != null)
                {
                    (<any>window).app._uiSculptingSize = value;
                    (<any>window).app._sculptingRadius = (<any>window).app._modelRadius * (<any>window).app._uiSculptingSize / 100.0;
                }
                return 'Current value: ' + value;
            }
        });
        // Bottom left
        document.getElementById("loadButton").onchange = function (event: Event)
        {
            let input = <HTMLInputElement>event.target;
            let files = input.files;
            if(files.length > 0)
            {
                let f: File = files[0];
                let reader = new FileReader();
                let filename = f.name;
                console.log("File name: " + filename);
                (<any>window).app.SwitchSpinner(true);
                reader.onload = function (e)
                {
                    let target: any = e.target;
                    let data = target.result;
                    let meshLoader: Module.MeshLoader = new Module.MeshLoader();
                    try
                    {
                        (<any>window).app.SwitchSpinner(false);
                        let meshItem: Module.Mesh = meshLoader.LoadFromTextBuffer(data, filename);
                        if(meshItem == null)
                        {
                            bootbox.alert({
                                title: "File loading failure",
                                message: "<p> Can't load file " + filename + "</p>",
                                backdrop: true
                            });
                        }
                        else(<any>window).app.SetMeshItemToScene(meshItem);
                    }
                    catch(err)
                    {
                        (<any>window).app.SwitchSpinner(false);
                        bootbox.alert({
                            title: "File loading failure",
                            message: "<p>The file " + filename + " fails to load. This is most of the time due to the lack of memory.</p><p>On the browser version the memory is <span style=\"text-decoration: underline;\">for the moment</span> limited. You shouldn't be able to load a mesh over one million triangles.</p><p>Please use Unity version if you want to load bigger meshes.</p>",
                            backdrop: true
                        });
                    }
                    meshLoader.delete();
                };
                reader.readAsArrayBuffer(f);
                input.value = "";   // To be able to load the same file again (as we are on a "onchange" callback)
            }
        };
        let saveButton: HTMLElement = <HTMLElement>document.getElementById("saveButton");
        saveButton.onclick = function ()
        {
            if(saveButton.getAttribute("userdata") == "password")
            {
                bootbox.prompt("To access this functionality, please enter your Dood access number", function (result)
                {
                    if(result == null)  // Cancel button was clicked
                        return;
                    let codes = ["villettemakerz", "598124", "482976", "197583", "978613", "846512", "465298", "246978", "383794", "641767", "282975"];
                    let matching: boolean = false;
                    for(let i = 0; i < codes.length; ++i)
                    {
                        if(result == codes[i])
                        {
                            matching = true;
                            break;
                        }
                    }
                    if(matching)
                    {
                        saveButton.setAttribute("userdata", "unlocked");    // Unlock the saving, and also this way the user won't have to enter password again
                        bootbox.dialog({
                            message: document.getElementById('ChoosOutputFormat').innerHTML,
                            backdrop: true,
                            onEscape: function () { }   // So that backdrop works on dialog
                        });
                    }
                    else
                    {
                        bootbox.alert({
                            title: "Wrong access number",
                            message: "<p> Sorry, but the number you entered is invalid. </p>",
                            backdrop: true
                        });
                    }
                });
            }
            else
            {
                bootbox.dialog({
                    message: document.getElementById('ChoosOutputFormat').innerHTML,
                    backdrop: true,
                    onEscape: function () { }   // So that backdrop works on dialog
                });
            }
        }
        let printButton: HTMLElement = <HTMLElement>document.getElementById("printButton");
        if(printButton != null)
        {
            printButton.onclick = function ()
            {
                var blob: Blob = (<any>window).app.SaveToBlob('stl');
                astroprint.importDesignByBlob(blob, "application/sla", "output.stl");
            }
        }
        document.getElementById("openButton").onclick = function ()
        {
            bootbox.dialog({
                message: document.getElementById('ChoseObject').innerHTML,
                backdrop: true,
                onEscape: function () { }   // So that backdrop works on dialog
            });
        };
        // Undo/redo
        document.getElementById("undoButton").onclick = function ()
        {
            (<any>window).app.GetCurMeshItem().Undo();
            (<any>window).app.UpdateBabylonMesh();
            (<any>window).app.UpdateUndoRedoButtonStates();
        };
        document.getElementById("redoButton").onclick = function ()
        {
            (<any>window).app.GetCurMeshItem().Redo();
            (<any>window).app.UpdateBabylonMesh();
            (<any>window).app.UpdateUndoRedoButtonStates();
        };
        // Combine mode related
        document.getElementById("moveButton").onclick = function ()
        {
            (<any>window).app._manipulator.SwitchToMode(ManipulatorMode.Move);
        };
        document.getElementById("rotateButton").onclick = function ()
        {
            (<any>window).app._manipulator.SwitchToMode(ManipulatorMode.Rotate);
        };
        document.getElementById("scaleButton").onclick = function ()
        {
            (<any>window).app._manipulator.SwitchToMode(ManipulatorMode.Scale);
        };
        document.getElementById("cancelCombineButton").onclick = function ()
        {
            (<any>window).app.StopCombineToSceneMode();
        };
        document.getElementById("mergeButton").onclick = function ()
        {
            (<any>window).app.CSGMerge();
        };
        document.getElementById("subtractButton").onclick = function ()
        {
            (<any>window).app.CSGSubtract();
        };
        document.getElementById("intersectButton").onclick = function ()
        {
            (<any>window).app.CSGIntersect();
        };
    }

    selectBrushType(brushType: BrushType, brushButton: HTMLElement)
    {
        this._brushType = brushType;
        // Unlight all brush buttons that are not selected, and light the other
        let brushMenu: HTMLElement = <HTMLElement>document.getElementById("brushMenu");
        if(brushMenu != null)
        {
            let curChild: HTMLElement = <HTMLElement>brushMenu.firstChild;
            while(curChild != null)
            {
                if((curChild.getAttribute != null) && (curChild.getAttribute("userdata") == "brushButton"))
                {
                    if(curChild != brushButton)
                    {
                        curChild.classList.remove("btn-primary");
                        curChild.classList.add("btn-default");
                    }
                    else
                    {
                        curChild.classList.add("btn-primary");
                        curChild.classList.remove("btn-default");
                    }
                }
                curChild = <HTMLElement>curChild.nextSibling;
            }
        }
    }

    changeColor(color: string)
    {
        let result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(color);
        let colorRed = parseInt(result[1], 16);
        let colorGreen = parseInt(result[2], 16);
        let colorBlue = parseInt(result[3], 16);
        (<BABYLON.StandardMaterial>this._mesh.material).unfreeze();
        if(color == "#F0DCC8")  // Dirty hack to put some special emissive on the "clay" color
            (<BABYLON.StandardMaterial>this._mesh.material).emissiveColor = new BABYLON.Color3(50.0 / 255.0, 25.0 / 255.0, 0.0 / 255.0);
        else(<BABYLON.StandardMaterial>this._mesh.material).emissiveColor = new BABYLON.Color3(colorRed / (255 * 15), colorGreen / (255 * 15), colorBlue / (255 * 15));
        (<BABYLON.StandardMaterial>this._mesh.material).diffuseColor = new BABYLON.Color3(colorRed / 255, colorGreen / 255, colorBlue / 255);
        setTimeout(function ()  // to let engine update material before freeing it
        {
            (<BABYLON.StandardMaterial>this._mesh.material).freeze();
        }, 100);
    }

    launchIntoFullscreen(element)
    {
        if(element.requestFullscreen)
            element.requestFullscreen();
        else if(element.mozRequestFullScreen)
            element.mozRequestFullScreen();
        else if(element.webkitRequestFullscreen)
            element.webkitRequestFullscreen();
        else if(element.msRequestFullscreen)
            element.msRequestFullscreen();
    }

    exitFullscreen()
    {
        if(document.exitFullscreen)
            document.exitFullscreen();
        else if(document.mozCancelFullScreen)
            document.mozCancelFullScreen();
        else if(document.webkitExitFullscreen)
            document.webkitExitFullscreen();
    }

    showBBoxes()
    {
        this._DEBUG_BoundingBoxRenderer.reset();
        let bboxes = this._meshItem.GetFragmentsBBox();
        for(let i = 0; i < bboxes.size(); ++i)
        {
            let bbox: Module.BBox = bboxes.get(i);
            let bMin: Module.Vector3 = bbox.Min();
            let bMax: Module.Vector3 = bbox.Max();
            this._DEBUG_BoundingBoxRenderer.renderList.push(new BABYLON.BoundingBox(new BABYLON.Vector3(bMin.X(), bMin.Y(), bMin.Z()), new BABYLON.Vector3(bMax.X(), bMax.Y(), bMax.Z())));
        }
        bboxes.delete();
    }

    CreateUIRingCursor()
    {
        // Create wireframe ring (diameter 1.0)
        let nbSegments: number = 100;
        let stepAngle: number = (2.0 * Math.PI) / nbSegments;
        let circleVertices: BABYLON.Vector3[] = [];
        let curAngle: number = 0.0;
        for(let i: number = 0; i <= nbSegments; ++i, curAngle += stepAngle)
            circleVertices.push(new BABYLON.Vector3(Math.cos(curAngle), 0.0, Math.sin(curAngle)));
        // Register it to Babylon
        this._uiRingCursor = BABYLON.Mesh.CreateLines("UIRingCursor", circleVertices, this._scene);
        this._uiRingCursor.renderingGroupId = 1;    // To avoid z-buffer test
        this._uiRingCursor.isVisible = false;   // Hidden while an object isn't set in the scene
    }

    ReadaptToModelSize()
    {
        // Get model size (to adjust sculpting radius)
        let firstMeshBBox: Module.BBox = this._meshItem.GetBBox();
        this._modelRadius = Math.max(Math.max(firstMeshBBox.Extents().X(), firstMeshBBox.Extents().Y()), firstMeshBBox.Extents().Z());
        this._sculptingRadius = this._modelRadius * this._uiSculptingSize / 100.0;
        let boundFirstMin: Module.Vector3 = firstMeshBBox.Min();
        let boundFirstMax: Module.Vector3 = firstMeshBBox.Max();
        let boundMin: BABYLON.Vector3 = new BABYLON.Vector3(boundFirstMin.X(), boundFirstMin.Y(), boundFirstMin.Z());
        let boundMax: BABYLON.Vector3 = new BABYLON.Vector3(boundFirstMax.X(), boundFirstMax.Y(), boundFirstMax.Z());
        firstMeshBBox.delete();
        if(this._isInCombineMode && this._meshToCreateOrCombine)
        {
            let secondMeshBoundingInfo: BABYLON.BoundingInfo = this._meshToCreateOrCombine.getBoundingInfo();
            let secondMeshMatrix = this._meshToCreateOrCombine.getWorldMatrix();
            let secondMeshMinBound = BABYLON.Vector3.TransformCoordinates(secondMeshBoundingInfo.minimum, secondMeshMatrix);
            let secondMeshMaxBound = BABYLON.Vector3.TransformCoordinates(secondMeshBoundingInfo.maximum, secondMeshMatrix);
            boundMin.x = Math.min(boundMin.x, secondMeshMinBound.x);
            boundMin.y = Math.min(boundMin.y, secondMeshMinBound.y);
            boundMin.z = Math.min(boundMin.z, secondMeshMinBound.z);
            boundMax.x = Math.max(boundMax.x, secondMeshMaxBound.x);
            boundMax.y = Math.max(boundMax.y, secondMeshMaxBound.y);
            boundMax.z = Math.max(boundMax.z, secondMeshMaxBound.z);
            let delta: BABYLON.Vector3 = boundMax.subtract(boundMin);
            this._modelRadius = Math.max(Math.max(delta.x, delta.y), delta.z) * 0.5;
        }

        // Retarget camera
        this._camera.setTarget(new BABYLON.Vector3((boundMin.x + boundMax.x) * 0.5, (boundMin.y + boundMax.y) * 0.5, (boundMin.z + boundMax.z) * 0.5));
        this._camera.minZ = 0;
        this._camera.radius = this._modelRadius * 3.4;
        this._camera.lowerRadiusLimit = this._camera.radius * 0.5;
        this._camera.upperRadiusLimit = this._camera.radius * 2.0;
    }

    UpdateUndoRedoButtonStates()
    {
        let undoButton: HTMLElement = <HTMLElement>document.getElementById("undoButton");
        if(undoButton != null)
        {
            if(this._meshItem.CanUndo())
                undoButton.classList.remove("disabled");
            else
                undoButton.classList.add("disabled");
        }
        let redoButton: HTMLElement = <HTMLElement>document.getElementById("redoButton");
        if(redoButton != null)
        {
            if(this._meshItem.CanRedo())
                redoButton.classList.remove("disabled");
            else
                redoButton.classList.add("disabled");
        }
    }

    GetCurMeshItem(): Module.Mesh
    {
        return this._meshItem;
    }

    GetCurMeshItemToCreateOrCombine(): Module.Mesh
    {
        return this._meshItemToCreateOrCombine;
    }

    ClearMeshItemToCreateOrCombine(eraseData: boolean)
    {
        if(this._meshItemToCreateOrCombine != null)
        {
            if(eraseData)
                this._meshItemToCreateOrCombine.delete();
            this._meshItemToCreateOrCombine = null;
        }
    }

    IsInCombineMode(): boolean
    {
        return this._isInCombineMode;
    }

    StartCombineToSceneMode()
    {
        if(this._meshItemToCreateOrCombine == null) // if there is no mesh to combine, prevent combine mode to start
            return;

        // Switch UI
        let combineUI: HTMLElement = <HTMLElement>document.getElementById("combineUI");
        let mainUI: HTMLElement = <HTMLElement>document.getElementById("mainUI");
        mainUI.style.display = "none";
        combineUI.style.display = "inline";
        // Hide ring cursor
        this._uiRingCursor.isVisible = false;
        // Create Babylon mesh
        let meshData: BABYLON.VertexData = new BABYLON.VertexData();
        let buf: Int8Array = this._meshItemToCreateOrCombine.Triangles();
        let triangles: Int32Array = new Int32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
        buf = this._meshItemToCreateOrCombine.Vertices();
        let vertices: Float32Array = new Float32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
        buf = this._meshItemToCreateOrCombine.Normals();
        let normals: Float32Array = new Float32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
        meshData.indices = triangles;
        meshData.positions = vertices;
        meshData.normals = normals;
        this._meshToCreateOrCombine = new BABYLON.Mesh("newMesh", this._scene);
        this._meshToCreateOrCombine.sideOrientation = BABYLON.Mesh.FRONTSIDE;
        this._meshToCreateOrCombine.isPickable = true;
        meshData.applyToMesh(this._meshToCreateOrCombine, false);
        // Position mesh to combine
        let bbox: Module.BBox = this._meshItem.GetBBox();
        let firstModelRadius: number = Math.max(Math.max(bbox.Extents().X(), bbox.Extents().Y()), bbox.Extents().Z());
        bbox.delete();
        bbox = this._meshItemToCreateOrCombine.GetBBox();
        let secondModelRadius: number = Math.max(Math.max(bbox.Extents().X(), bbox.Extents().Y()), bbox.Extents().Z());
        bbox.delete();
        bbox = null;
        let positionShift: number = firstModelRadius + secondModelRadius;
        let position: BABYLON.Vector3 = BABYLON.Vector3.Right(); //this._camera.getWorldMatrix().getRow(0).toVector3();
        position.scaleInPlace(positionShift);
        this._meshToCreateOrCombine.position = position;
        this._meshToCreateOrCombine.computeWorldMatrix(true);
        this._isInCombineMode = true;        
        this.ReadaptToModelSize();
        // Set manipulator to handle that object
        this._manipulator.ForceMeshSelection(this._meshToCreateOrCombine);
        this._manipulator.Start();
    }

    StopCombineToSceneMode()
    {
        let combineUI: HTMLElement = <HTMLElement>document.getElementById("combineUI");
        let mainUI: HTMLElement = <HTMLElement>document.getElementById("mainUI");
        mainUI.style.display = "inline";
        combineUI.style.display = "none";
        this.ClearMeshItemToCreateOrCombine(true);
        this._manipulator.Stop();
        this._meshToCreateOrCombine.dispose();
        this._meshToCreateOrCombine = null;
        this._uiRingCursor.isVisible = true;
        this._isInCombineMode = false;
        this.ReadaptToModelSize();
    }

    DoCSGOperation(opType: CsgType)
    {
        this.SwitchSpinner(true);
        var that = this;
        setTimeout(function ()  // setTimeout() so that UI has time to update and display the spinner
        {
            let babWorldMtx = that._meshToCreateOrCombine.getWorldMatrix();
            let otherMeshPos: Module.Vector3 = new Module.Vector3(babWorldMtx.getTranslation().x, babWorldMtx.getTranslation().y, babWorldMtx.getTranslation().z);
            let otherMeshRight: Module.Vector3 = new Module.Vector3(babWorldMtx.getRow(0).x, babWorldMtx.getRow(1).x, babWorldMtx.getRow(2).x);
            let otherMeshUp: Module.Vector3 = new Module.Vector3(babWorldMtx.getRow(0).y, babWorldMtx.getRow(1).y, babWorldMtx.getRow(2).y);
            let otherMeshFront: Module.Vector3 = new Module.Vector3(babWorldMtx.getRow(0).z, babWorldMtx.getRow(1).z, babWorldMtx.getRow(2).z);
            let otherMeshRotAndScale: Module.Matrix3 = new Module.Matrix3(otherMeshRight, otherMeshUp, otherMeshFront);
            switch(opType)
            {
                case CsgType.Merge:
                    that._meshItem.CSGMerge(that._meshItemToCreateOrCombine, otherMeshRotAndScale, otherMeshPos, true);
                    break;
                case CsgType.Subtract:
                    that._meshItem.CSGSubtract(that._meshItemToCreateOrCombine, otherMeshRotAndScale, otherMeshPos, true);
                    break;
                case CsgType.Intersect:
                    that._meshItem.CSGIntersect(that._meshItemToCreateOrCombine, otherMeshRotAndScale, otherMeshPos, true);
                    break;
            }
            that.StopCombineToSceneMode();
            that.UpdateBabylonMesh();
            that.UpdateUndoRedoButtonStates();
            that.SwitchSpinner(false);
            otherMeshPos.delete();
            otherMeshRight.delete();
            otherMeshUp.delete();
            otherMeshFront.delete();
            otherMeshRotAndScale.delete();
        }, 100);
    }

    CSGMerge()
    {
        this.DoCSGOperation(CsgType.Merge);
    }

    CSGSubtract()
    {
        this.DoCSGOperation(CsgType.Subtract);
    }

    CSGIntersect()
    {
        this.DoCSGOperation(CsgType.Intersect);
    }

    SwitchSpinner(switchOnOff: boolean)
    {
        let spinner: HTMLInputElement = <HTMLInputElement>document.getElementById('spinner');
        if(spinner != null)
        {
            spinner.style.display = switchOnOff ? "inline" : "none";
        }
    }

    SetMeshItemToScene(meshItem: Module.Mesh)
    {
        var that = this;
        function DoStuffAfterManifoldTest()
        {
            if((that._meshItem == null) || (that._booleanOn == false))  // First scene, then no need to prompt for merging or creating a new scene
                that.CreateNewScene(meshItem);
            else
            {   // Have to prompt if user want to create a new scene or merge into existing one
                that._meshItemToCreateOrCombine = meshItem;
                bootbox.dialog({
                    message: document.getElementById('CreateOrCombine').innerHTML,
                    backdrop: true,
                    onEscape: function ()
                    {
                        that.ClearMeshItemToCreateOrCombine(true);
                    }
                });
            }
        }
        if(meshItem.IsManifold() == false)
        {
            bootbox.alert({
                title: "Hazardous mesh input",
                message: "<p>Input mesh is not a closed manifold mesh.</p><p>(A manifold mesh is a mesh that could be represented in \"real life\")</p><p>You will be able to edit the mesh, but you may encounter side effects.</p>",
                backdrop: true,
                callback: function ()
                {
                    DoStuffAfterManifoldTest();
                },
                onEscape: function ()
                {
                    DoStuffAfterManifoldTest();
                }
            });
        }
        else
            DoStuffAfterManifoldTest();
    }

    CreateNewScene(meshItem: Module.Mesh)
    {
        if(this._meshItem != null)
            this._meshItem.delete();
        this._meshItem = meshItem;
        //this._meshItem.SetBBoxRenderer(this._DEBUG_BoundingBoxRenderer);
        if(this._brushDraw != null) this._brushDraw.delete();
        this._brushDraw = new Module.BrushDraw(this._meshItem);
        if(this._brushInflate != null) this._brushInflate.delete();
        this._brushInflate = new Module.BrushInflate(this._meshItem);
        if(this._brushFlatten != null) this._brushFlatten.delete();
        this._brushFlatten = new Module.BrushFlatten(this._meshItem);
        if(this._brushDrag != null) this._brushDrag.delete();
        this._brushDrag = new Module.BrushDrag(this._meshItem);
        if(this._brushDig != null) this._brushDig.delete();
        this._brushDig = new Module.BrushDig(this._meshItem);

        this._material = new BABYLON.StandardMaterial("meshMaterial", this._scene);
        //this._scene.ambientColor = new BABYLON.Color3(0.4, 0.4, 0.4);
        //this._material.ambientColor = new BABYLON.Color3(90.0 / 255.0, 90.0 / 255.0, 90.0 / 255.0);
        this._material.emissiveColor = new BABYLON.Color3(50.0 / 255.0, 25.0 / 255.0, 0.0 / 255.0);
        this._material.diffuseColor = new BABYLON.Color3(240.0 / 255.0, 220.0 / 255.0, 200.0 / 255.0);
        this._material.specularColor = new BABYLON.Color3(200.0 / 255.0, 200.0 / 255.0, 200.0 / 255.0);
        this._material.specularPower = 1000;
        this._material.backFaceCulling = true;
        this._material.freeze();

        if(this._useSubMeshes == false)
        {
            let meshData: BABYLON.VertexData = new BABYLON.VertexData();
            let buf: Int8Array = this._meshItem.Triangles();
            let triangles: Int32Array = new Int32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
            buf = this._meshItem.Vertices();
            let vertices: Float32Array = new Float32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
            buf = this._meshItem.Normals();
            let normals: Float32Array = new Float32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
            meshData.indices = triangles;
            meshData.positions = vertices;
            meshData.normals = normals;
            if(this._mesh == null)
            {
                this._mesh = new BABYLON.Mesh("mesh", this._scene);
                this._mesh.freezeWorldMatrix();
                this._mesh.material = this._material;
                this._mesh.material.freeze();
            }
            this._mesh.sideOrientation = BABYLON.Mesh.FRONTSIDE;
            this._mesh.isPickable = false;
            meshData.applyToMesh(this._mesh, true);
        }
        else
            this.UpdateBabylonMesh();

        // Update model size, sculpting radius, camera placement
        this.ReadaptToModelSize();

        // Undo/redo
        this.UpdateUndoRedoButtonStates();

        // Show cursor
        this._uiRingCursor.isVisible = true;
    }

    GenSphere()
    {
        let generator: Module.GenSphere = new Module.GenSphere();
        (<any>window).app.SetMeshItemToScene(generator.Generate(100.0));
        generator.delete();
    }

    GenBox()
    {
        let generator: Module.GenBox = new Module.GenBox();
        (<any>window).app.SetMeshItemToScene(generator.Generate(180.0, 100.0, 100.0));
        generator.delete();
    }

    GenCylinder()
    {
        let generator: Module.GenCylinder = new Module.GenCylinder();
        (<any>window).app.SetMeshItemToScene(generator.Generate(180.0, 50.0));
        generator.delete();
    }

    GenCube()
    {
        let generator: Module.GenBox = new Module.GenBox();
        (<any>window).app.SetMeshItemToScene(generator.Generate(100.0, 100.0, 100.0));
        generator.delete();
    }

    GenPyramid()
    {
        let generator: Module.GenPyramid = new Module.GenPyramid();
        (<any>window).app.SetMeshItemToScene(generator.Generate(100.0, 150.0, 100.0));
        generator.delete();
    }

    GenDisk()
    {
        let generator: Module.GenCylinder = new Module.GenCylinder();
        (<any>window).app.SetMeshItemToScene(generator.Generate(10.0, 100.0));
        generator.delete();
    }

    Load3DModel(filename: string)
    {
        this.SwitchSpinner(true);
        let oReq = new XMLHttpRequest();
        oReq.open("GET", "./3d_models/" + filename, true);
        oReq.responseType = "arraybuffer";
        oReq.onload = function (oEvent)
        {
            let arrayBuffer: ArrayBuffer = oReq.response; // Note: not oReq.responseText
            let meshLoader: Module.MeshLoader = new Module.MeshLoader();
            (<any>window).app.SetMeshItemToScene(meshLoader.LoadFromTextBuffer(arrayBuffer, filename));
            meshLoader.delete();
            (<any>window).app.SwitchSpinner(false);
        };
        oReq.send(null);
    }

    SaveToFile(fileExt: string)
    {
        let saveButton: HTMLElement = <HTMLElement>document.getElementById("saveButton");
        if(saveButton.getAttribute("userdata") != "unlocked")
        {
            bootbox.alert({
                title: "Saving to file is turned off",
                message: "<p>This is a demo version, that's why saving to file is turned off.</p><p>If you're interested with what you've seen, please visit <a href=\"http://www.tectrid.com/contact \" target=\"_blank\">contact</a> page.</p>",
                backdrop: true
            });
        }
        else
        {
            saveAs(this.SaveToBlob(fileExt), "output." + fileExt);
        }
    }

    SaveToBlob(fileExt: string): Blob
    {
        let bbox: Module.BBox = this._meshItem.GetBBox();
        let bboxSize: Module.Vector3 = bbox.Size();
        let maxSideSize = Math.max(bboxSize.X(), Math.max(bboxSize.Y(), bboxSize.Z()));
        let scaleToTenCentimeter = 100.0 / maxSideSize;

        let meshRecorder: Module.MeshRecorder = new Module.MeshRecorder();
        let babMeshTransform = BABYLON.Matrix.RotationYawPitchRoll(0, Math.PI * 0.5, 0).multiply(BABYLON.Matrix.Scaling(scaleToTenCentimeter, scaleToTenCentimeter, scaleToTenCentimeter));
        let meshTransformRight: Module.Vector3 = new Module.Vector3(babMeshTransform.getRow(0).x, babMeshTransform.getRow(1).x, babMeshTransform.getRow(2).x);
        let meshTransformUp: Module.Vector3 = new Module.Vector3(babMeshTransform.getRow(0).y, babMeshTransform.getRow(1).y, babMeshTransform.getRow(2).y);
        let meshTransformFront: Module.Vector3 = new Module.Vector3(babMeshTransform.getRow(0).z, babMeshTransform.getRow(1).z, babMeshTransform.getRow(2).z);
        let meshTransform: Module.Matrix3 = new Module.Matrix3(meshTransformRight, meshTransformUp, meshTransformFront);
        meshRecorder.SetTransformMatrix(meshTransform);
        let objStringFileData: string = meshRecorder.SaveToTextBuffer((<any>window).app.GetCurMeshItem(), fileExt);
        meshRecorder.delete();
        let objBinFileData: Uint8Array = new Uint8Array(objStringFileData.length);
        for(let i: number = 0; i < objStringFileData.length; ++i)
            objBinFileData[i] = objStringFileData.charCodeAt(i);
        return new Blob(<any>[objBinFileData], { type: "application/octet-binary" });
    }

    UpdateBabylonMesh()
    {
        if(this._useSubMeshes)
        {
            // Update sub meshes internal structure
            this._meshItem.UpdateSubMeshes();
            // Update it to Babylon
            for(let i: number = 0; i < this._meshItem.GetSubMeshCount(); ++i)
            {
                let submesh: Module.SubMesh = this._meshItem.GetSubMesh(i);
                let subMeshID: string = submesh.GetID().toString();
                if(this._babylonSubMeshes.hasOwnProperty(subMeshID))
                    this._babylonSubMeshes[subMeshID].Update();
                else
                    this._babylonSubMeshes[subMeshID] = new BabylonSubMesh(this._scene, null, submesh, this._material);
            }
            // Remove submeshes that were deleted
            for(var key in this._babylonSubMeshes)
            {
                if(this._babylonSubMeshes.hasOwnProperty(key))
                {
                    if(this._meshItem.IsSubMeshExist(this._babylonSubMeshes[key].GetID()) == false)
                    {
                        this._babylonSubMeshes[key].Cleanup();
                        delete this._babylonSubMeshes[key];
                    }
                }
            }
        }
        else
        {
            let buf: Int8Array = this._meshItem.Triangles();
            let triangles: Uint32Array = new Uint32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
            buf = this._meshItem.Vertices();
            let vertices: Float32Array = new Float32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
            buf = this._meshItem.Normals();
            let normals: Float32Array = new Float32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);

            // Vertices and normals
            if(this._mesh._geometry.getTotalVertices() < vertices.length / 3)
            {
                this._mesh.setVerticesData(BABYLON.VertexBuffer.PositionKind, vertices, true);
                this._mesh.setVerticesData(BABYLON.VertexBuffer.NormalKind, normals, true);
            }
            else
            {
                this._mesh.updateVerticesData(BABYLON.VertexBuffer.PositionKind, vertices, false, false);
                this._mesh.updateVerticesData(BABYLON.VertexBuffer.NormalKind, normals, false, false);
            }

            // Triangle indices
            this._mesh.setIndices(triangles);
        }
    }

    createScene()
    {
        // create a basic BJS Scene object
        this._scene = new BABYLON.Scene(this._engine);
        this._scene.useRightHandedSystem = true;
		//this._DEBUG_BoundingBoxRenderer = new BABYLON.BoundingBoxRenderer(this._scene);
        //this._scene.debugLayer.show(true);
        //this._scene.clearColor = new BABYLON.Color3(0.2, 0.2, 0.2);

        // create a FreeCamera, and set its position to (x:0, y:5, z:-10)
        this._camera = new BABYLON.ArcRotateCamera("Camera", Math.PI / 2, 0.8, 3, BABYLON.Vector3.Zero(), this._scene);
		this._camera.lowerRadiusLimit = 3;
        this._camera.upperRadiusLimit = 3;

        // target the camera to scene origin
        this._camera.setTarget(BABYLON.Vector3.Zero());

        // attach the camera to the canvas
        this._camera.attachControl(this._canvas, false);

        // create a basic light, aiming 0,1,0 - meaning, to the sky
        this._light = new BABYLON.PointLight('light1', new BABYLON.Vector3(0, 1, 0), this._scene);
        //this._light.diffuse = new BABYLON.Color3(1.0, 0.0, 0.0);

        // Ring cursor
        this.CreateUIRingCursor();

        // Create move, rot, scale manipulator
        this._manipulator = new Manipulator(this._engine, this._scene, this._camera);

        // Create starting model
        Module.SculptEngine.SetTriangleOrientationInverted(true);
        let modelToLoad = getQueryStringValue("model");
        switch(modelToLoad)
        {
            case "sphere":
                this.GenSphere();
                break;
            case "box":
                this.GenBox();
                break;
            case "cylinder":
                this.GenCylinder();
                break;
            case "cube":
                this.GenCube();
                break;
            case "pyramid":
                this.Load3DModel('pyramid.tct');
                break;
            case "disk":
                this.Load3DModel('disk.tct');
                break;
            case "ring":
                this.Load3DModel('ring.tct');
                break;
            case "mug":
                this.Load3DModel('mug.tct');
                break;
            case "head":
                this.Load3DModel('head.tct');
                break;
            case "girl":
                this.Load3DModel('girl.tct');
                break;
            case "boy":
                this.Load3DModel('boy.tct');
                break;
            case "cat":
                this.Load3DModel('cat.tct');
                break;
            case "gearknob":
                this.Load3DModel('gearknob.tct');
                break;
            default:
                this.GenSphere();
        }

        // Bbox update
		//this.showBBoxes();

        // Set the target of the camera to the first imported mesh
        if((this._useSubMeshes == false) && (this._meshItem != null))
            this._camera.target = this._mesh.position;
        else
            this._camera.target = BABYLON.Vector3.Zero();

        // Set background color
        this._scene.clearColor.r = 26 / 255;
        this._scene.clearColor.g = 26 / 255;
        this._scene.clearColor.b = 24 / 255;

        // Move the light with the camera
        this._scene.registerBeforeRender(() =>
        {
            this._light.position = this._camera.position;
            if(this.IsInCombineMode())
            {
            }
            else if(this._meshItem != null)
            {
                /*let mat: BABYLON.Matrix = this._camera.getWorldMatrix();
                this._light.position = this._camera.position.add(mat.getRow(0).toVector3().scale(this._modelRadius * 2)).add(mat.getRow(1).toVector3().scale(this._modelRadius * 2));*/
                if(this._sculptPoint != null)
                {
                    this.Sculpt(this._sculptPoint);
                    if(this._brushType != BrushType.Drag)	// Don't need to move cursor when dragging
                        this._sculptPoint = null;
                }
                // Update cursor
                let babRay: BABYLON.Ray = this._scene.createPickingRay(this._uiCursorScreenPos.x, this._uiCursorScreenPos.y, null, this._camera);
                let intersection: Module.Vector3 = new Module.Vector3(0, 0, 0);
                let intersectionNormal: Module.Vector3 = new Module.Vector3(0, 0, 0);
                let rayOrigin = new Module.Vector3(babRay.origin.x, babRay.origin.y, babRay.origin.z);
                let rayDirection = new Module.Vector3(babRay.direction.x, babRay.direction.y, babRay.direction.z);
                let ray = new Module.Ray(rayOrigin, rayDirection, this._rayLength);
                if(this._meshItem.GetClosestIntersectionPoint(ray, intersection, intersectionNormal, true))
                {
                    let at: BABYLON.Vector3 = new BABYLON.Vector3(intersectionNormal.X(), intersectionNormal.Y(), intersectionNormal.Z());
                    let up: BABYLON.Vector3 = BABYLON.Vector3.Up(); // 0, 1, 0
                    if(Math.abs(BABYLON.Vector3.Dot(up, at)) > 0.95)
                        up = BABYLON.Vector3.Right(); // 1, 0, 0
                    let left: BABYLON.Vector3 = BABYLON.Vector3.Cross(up, at);
                    left.normalize();
                    up = BABYLON.Vector3.Cross(left, at);
                    up.normalize();
                    let mat: BABYLON.Matrix = new BABYLON.Matrix();
                    BABYLON.Matrix.FromXYZAxesToRef(left.scale(this._sculptingRadius), at.scale(this._sculptingRadius), up.scale(this._sculptingRadius), mat);
                    mat.setTranslation(new BABYLON.Vector3(intersection.X(), intersection.Y(), intersection.Z()));
                    this._uiRingCursor.setPivotMatrix(mat);
                }
                else
                {
                    let uiRingPos: BABYLON.Vector3 = babRay.origin.add(babRay.direction.scale(babRay.origin.length()));
                    let mat: BABYLON.Matrix = this._camera.getWorldMatrix().clone();
                    let row0: BABYLON.Vector4 = mat.getRow(0).scaleInPlace(this._sculptingRadius);
                    let row1: BABYLON.Vector4 = mat.getRow(1).scaleInPlace(this._sculptingRadius);
                    let row2: BABYLON.Vector4 = mat.getRow(2).scaleInPlace(this._sculptingRadius);
                    mat.setRow(0, row1);
                    mat.setRow(1, row2);
                    mat.setRow(2, row0);
                    mat.setTranslation(uiRingPos);
                    this._uiRingCursor.setPivotMatrix(mat);
                }
            }
        });

		// input events
		this._scene.onPointerDown = (e, p) =>
        {
            if(this.IsInCombineMode())
            {
                this._manipulator.onPointerDown(e, p);
            }
            else if(this._meshItem != null)
            {
                let babRay: BABYLON.Ray = this._scene.createPickingRay(e.clientX, e.clientY, null, this._camera);
                let intersection: Module.Vector3 = new Module.Vector3(0, 0, 0);
                let rayOrigin = new Module.Vector3(babRay.origin.x, babRay.origin.y, babRay.origin.z);
                let rayDirection = new Module.Vector3(babRay.direction.x, babRay.direction.y, babRay.direction.z);
                let ray = new Module.Ray(rayOrigin, rayDirection, this._rayLength);
                // we pick an object, activate sculpting
                if(this._meshItem.GetClosestIntersectionPoint(ray, intersection, null, true))
                {
                    this._camera.detachControl(this._canvas);
                    this._sculpting = true;
                    switch(this._brushType)
                    {
                        case BrushType.Draw:
                            this._brushDraw.StartStroke();
                            break;
                        case BrushType.Inflate:
                            this._brushInflate.StartStroke();
                            break;
                        case BrushType.Flatten:
                            this._brushFlatten.StartStroke();
                            break;
                        case BrushType.Drag:
                            this._brushDrag.StartStroke();
                            break;
                        case BrushType.Dig:
                            this._brushDig.StartStroke();
                            break;
                    }
                    this._lastSculptPoint = new BABYLON.Vector2(e.clientX, e.clientY);
                }
                ray.delete();
                rayDirection.delete();
                rayOrigin.delete();
                intersection.delete();
            }
		}
		this._scene.onPointerMove = (e, p) =>
        {
            if(this.IsInCombineMode())
            {
                this._manipulator.onPointerMove(e, p);
            }
            else if(this._meshItem != null)
            {
                if(this._sculpting)
                    this._sculptPoint = new BABYLON.Vector2(e.clientX, e.clientY);
                this._uiCursorScreenPos.x = e.clientX;
                this._uiCursorScreenPos.y = e.clientY;
                if(((e.buttons & 1) == 0) && this._sculpting)    // Left mouse button was released: if it was released when cursos was out of a iframe (if the webapp sits in a iframe), then we wouldn't get the mouse up message
                    this._scene.onPointerUp(e, p);
            }
 		}
        this._scene.onPointerUp = (e, p) =>
        {
            if(this.IsInCombineMode())
            {
                this._manipulator.onPointerUp(e, p);
            }
            else if(this._meshItem != null)
            {
                if(this._sculpting)
                {
                    this._camera.attachControl(this._canvas, true);
                    this._sculpting = false;
                    this._sculptPoint = null;
                    switch(this._brushType)
                    {
                        case BrushType.Draw:
                            this._brushDraw.EndStroke();
                            break;
                        case BrushType.Inflate:
                            this._brushInflate.EndStroke();
                            break;
                        case BrushType.Flatten:
                            this._brushFlatten.EndStroke();
                            break;
                        case BrushType.Drag:
                            this._brushDrag.EndStroke();
                            break;
                        case BrushType.Dig:
                            this._brushDig.EndStroke();
                            break;
                    }
                    // Update model size, sculpting radius, camera placement
                    //this.ReadaptToModelSize();
                    // Undo/redo
                    this.UpdateUndoRedoButtonStates();
                }
            }
        }
    }

    animate(): void
    {
        // run the render loop
        this._engine.runRenderLoop(() =>
		{
            this._scene.render();
			//this._DEBUG_BoundingBoxRenderer.render();
        });

        // the canvas/window resize event handler
        window.addEventListener('resize', () =>
		{
            this._engine.resize();
        });
    }

    private clamp(value, min, max)
    {
        return Math.min(Math.max(value, min), max);
    }

    public Sculpt(screenPoint: BABYLON.Vector2)
    {
        let babRay: BABYLON.Ray = this._scene.createPickingRay(screenPoint.x, screenPoint.y, null, this._camera);
        let intersection: Module.Vector3 = new Module.Vector3(0, 0, 0);
        let rayOrigin = new Module.Vector3(babRay.origin.x, babRay.origin.y, babRay.origin.z);
        let rayDirection = new Module.Vector3(babRay.direction.x, babRay.direction.y, babRay.direction.z);
        let ray = new Module.Ray(rayOrigin, rayDirection, this._rayLength);
        switch(this._brushType)
        {
            case BrushType.Draw:
                this._brushDraw.UpdateStroke(ray, this._sculptingRadius, this._sculptingStrengthRatio);
                break;
            case BrushType.Inflate:
                this._brushInflate.UpdateStroke(ray, this._sculptingRadius, this._sculptingStrengthRatio);
                break;
            case BrushType.Flatten:
                this._brushFlatten.UpdateStroke(ray, this._sculptingRadius, this._sculptingStrengthRatio);
                break;
            case BrushType.Drag:
                this._brushDrag.UpdateStroke(ray, this._sculptingRadius, this._sculptingStrengthRatio);
                break;
            case BrushType.Dig:
                this._brushDig.UpdateStroke(ray, this._sculptingRadius, this._sculptingStrengthRatio);
                break;
        }
        ray.delete();
        rayDirection.delete();
        rayOrigin.delete();
        intersection.delete();

        // Mesh update
        this.UpdateBabylonMesh();

        // Bbox update
		//this.showBBoxes();
	}
}

function StartAppMain()
{
    // Create the game using the 'renderCanvas'
    (<any>window).app = new AppMain('renderCanvas');

    // Create the scene
    (<any>window).app.createScene();

    // Stop spinner now app is fully loaded and initialized
    (<any>window).app.SwitchSpinner(false);

    // start animation
    (<any>window).app.animate();
}

/*window.addEventListener('DOMContentLoaded', () =>
{
    StartAppMain();
});*/
