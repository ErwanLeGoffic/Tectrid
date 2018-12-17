enum ManipulatorMode
{
    Move,
    Rotate,
    Scale
}

class Manipulator
{
    private _scene: BABYLON.Scene;
    private _engine: BABYLON.Engine;
    private _camera: BABYLON.ArcRotateCamera;
    private _pickResult: BABYLON.PickingInfo;
    private _pickDeltaFromMeshOrigin: BABYLON.Vector3;
    private _forcedMeshSelect: BABYLON.Mesh;
    private _curSelectedMesh: BABYLON.Mesh;
    private _selectedMeshMaterial: BABYLON.StandardMaterial;
    private _xDirMeshMaterial: BABYLON.StandardMaterial;
    private _yDirMeshMaterial: BABYLON.StandardMaterial;
    private _zDirMeshMaterial: BABYLON.StandardMaterial;
    private _allDirMeshMaterial: BABYLON.StandardMaterial;
    private _mode: ManipulatorMode;
    private _meshScaleX: BABYLON.Mesh;
    private _meshScaleY: BABYLON.Mesh;
    private _meshScaleZ: BABYLON.Mesh;
    private _meshScaleAll: BABYLON.Mesh;
    private _initialScale: BABYLON.Vector3;
    private _meshRotPitch: BABYLON.Mesh;
    private _meshRotYaw: BABYLON.Mesh;
    private _meshRotRoll: BABYLON.Mesh;
    private _initialRot: BABYLON.Quaternion;

    public constructor(engine: BABYLON.Engine, scene: BABYLON.Scene, camera: BABYLON.ArcRotateCamera)
    {
        this._scene = scene;
        this._engine = engine;
        this._camera = camera;
        this._selectedMeshMaterial = new BABYLON.StandardMaterial("meshMaterial", this._scene);
        this._selectedMeshMaterial.ambientColor = new BABYLON.Color3(0.0 / 255.0, 0.0 / 255.0, 0.0 / 255.0);
        this._selectedMeshMaterial.emissiveColor = new BABYLON.Color3(0.0 / 255.0, 0.0 / 255.0, 255.0 / 255.0);
        this._selectedMeshMaterial.diffuseColor = new BABYLON.Color3(255.0 / 255.0, 255.0 / 255.0, 255.0 / 255.0);
        this._selectedMeshMaterial.specularColor = new BABYLON.Color3(200.0 / 255.0, 200.0 / 255.0, 200.0 / 255.0);
        this._selectedMeshMaterial.specularPower = 1000;
        this._selectedMeshMaterial.backFaceCulling = true;
        this._selectedMeshMaterial.freeze();
        this._xDirMeshMaterial = new BABYLON.StandardMaterial("xDirMeshMaterial", this._scene);
        this._xDirMeshMaterial.diffuseColor = new BABYLON.Color3(1.0, 0.0, 0.0);
        this._xDirMeshMaterial.freeze();
        this._yDirMeshMaterial = new BABYLON.StandardMaterial("yDirMeshMaterial", this._scene);
        this._yDirMeshMaterial.diffuseColor = new BABYLON.Color3(0.0, 1.0, 0.0);
        this._yDirMeshMaterial.freeze();
        this._zDirMeshMaterial = new BABYLON.StandardMaterial("zDirMeshMaterial", this._scene);
        this._zDirMeshMaterial.diffuseColor = new BABYLON.Color3(0.0, 0.0, 1.0);
        this._zDirMeshMaterial.freeze();
        this._allDirMeshMaterial = new BABYLON.StandardMaterial("allDirMeshMaterial", this._scene);
        this._allDirMeshMaterial.diffuseColor = new BABYLON.Color3(1.0, 1.0, 0.0);
        this._allDirMeshMaterial.freeze();
        this._mode = ManipulatorMode.Move;
    }

    public Start()
    {
        this._mode = ManipulatorMode.Move;
        this.StartMode(this._mode);
    }

    public Stop()
    {
        this.StopMode(this._mode);
        this._curSelectedMesh = null;
        this._forcedMeshSelect = null;
    }
    
    public onPointerDown(e: PointerEvent, p: BABYLON.PickingInfo)
    {
        let pickResult: BABYLON.PickingInfo = p; //this._scene.pick(e.clientX, e.clientY);
        switch(this._mode)
        {
            case ManipulatorMode.Move:
                if((pickResult.pickedMesh != null) && ((pickResult.pickedMesh == this._forcedMeshSelect) || (this._forcedMeshSelect == null)))
                {   // Activate mesh dragging
                    this._pickResult = pickResult;
                    this._pickDeltaFromMeshOrigin = pickResult.pickedPoint.subtract(pickResult.pickedMesh.position);    // Get delta from pick point to mesh origin, will be used when moving cursor
                    this._camera.detachControl(this._engine.getRenderingCanvas());   // Block camera
                }
                break;
            case ManipulatorMode.Rotate:
                if((pickResult.pickedMesh != null) && ((pickResult.pickedMesh == this._meshRotPitch) || (pickResult.pickedMesh == this._meshRotYaw) || (pickResult.pickedMesh == this._meshRotRoll)))
                {   // Activate rotating
                    this._pickResult = pickResult;
                    this._pickDeltaFromMeshOrigin = pickResult.pickedPoint.subtract(pickResult.pickedMesh.position);    // Get delta from pick point to manipulator origin, will be used when rotating on pitch/yaw
                    this._camera.detachControl(this._engine.getRenderingCanvas());   // Block camera
                    if(this._curSelectedMesh.rotationQuaternion != null)
                        this._initialRot = this._curSelectedMesh.rotationQuaternion.clone();
                    else
                        this._initialRot = new BABYLON.Quaternion();
                }
                break;
            case ManipulatorMode.Scale:
                if((pickResult.pickedMesh != null) && ((pickResult.pickedMesh == this._meshScaleX) || (pickResult.pickedMesh == this._meshScaleY) || (pickResult.pickedMesh == this._meshScaleZ) || (pickResult.pickedMesh == this._meshScaleAll)))
                {   // Activate scaling
                    this._pickResult = pickResult;
                    this._camera.detachControl(this._engine.getRenderingCanvas());   // Block camera
                    this._initialScale = this._curSelectedMesh.scaling.clone();
                }
                break;
        }
    }

    public onPointerMove(e: PointerEvent, p: BABYLON.PickingInfo)
    {
        if((this._pickResult != null) && (this._pickResult.pickedMesh != null))
        {
            switch(this._mode)
            {
                case ManipulatorMode.Move:
                    {
                        let curPickRay: BABYLON.Ray = this._scene.createPickingRay(e.clientX, e.clientY, null, this._camera);
                        // Project current object position onto mouse ray
                        let origToCurPos = this._pickResult.pickedMesh.position.add(this._pickDeltaFromMeshOrigin).subtract(curPickRay.origin);
                        let projPos: BABYLON.Vector3 = curPickRay.origin.add(curPickRay.direction.scale(origToCurPos.length()));
                        this._pickResult.pickedMesh.position = projPos.subtract(this._pickDeltaFromMeshOrigin);
                    }
                    break;
                case ManipulatorMode.Rotate:
                    {
                        let curPickRay: BABYLON.Ray = this._scene.createPickingRay(e.clientX, e.clientY, null, this._camera);
                        let curDraggedPickPoint: BABYLON.Vector3 = curPickRay.origin.add(curPickRay.direction.scale(this._pickResult.distance));
                        let updateManipulatorsTransforms: boolean = false;
                        let dirMeshOriginToPickOrigin: BABYLON.Vector3 = this._pickResult.pickedPoint.subtract(this._curSelectedMesh.position).normalize();
                        let dirMeshOriginToCurDraggedPoint: BABYLON.Vector3 = curDraggedPickPoint.subtract(this._curSelectedMesh.position).normalize();
                        let dot: number = BABYLON.Vector3.Dot(dirMeshOriginToPickOrigin, dirMeshOriginToCurDraggedPoint);
                        if(dot < 1.0)
                        {
                            let angle: number = Math.acos(dot);
                            let cross: BABYLON.Vector3 = BABYLON.Vector3.Cross(dirMeshOriginToPickOrigin, dirMeshOriginToCurDraggedPoint);
                            if(this._pickResult.pickedMesh == this._meshRotPitch)
                            {
                                // Check if we are rotating clockwize or not and modify angle accordingly
                                let xAxis: BABYLON.Vector3 = this._curSelectedMesh.getWorldMatrix().getRow(0).toVector3();
                                xAxis.normalize();
                                if(BABYLON.Vector3.Dot(xAxis, cross) < 0.0)
                                    angle = -angle;
                                // Rotate selected mesh
                                let pitchQuat: BABYLON.Quaternion = BABYLON.Quaternion.RotationYawPitchRoll(0.0, angle, 0.0);
                                this._curSelectedMesh.rotationQuaternion = this._initialRot.multiply(pitchQuat);
                                updateManipulatorsTransforms = true;
                            }
                            else if(this._pickResult.pickedMesh == this._meshRotYaw)
                            {
                                // Check if we are rotating clockwize or not and modify angle accordingly
                                let yAxis: BABYLON.Vector3 = this._curSelectedMesh.getWorldMatrix().getRow(1).toVector3();
                                yAxis.normalize();
                                if(BABYLON.Vector3.Dot(yAxis, cross) < 0.0)
                                    angle = -angle;
                                // Rotate selected mesh
                                let yawQuat: BABYLON.Quaternion = BABYLON.Quaternion.RotationYawPitchRoll(angle, 0.0, 0.0);
                                this._curSelectedMesh.rotationQuaternion = this._initialRot.multiply(yawQuat);
                                updateManipulatorsTransforms = true;
                            }
                            else if(this._pickResult.pickedMesh == this._meshRotRoll)
                            {
                                // Check if we are rotating clockwize or not and modify angle accordingly
                                let zAxis: BABYLON.Vector3 = this._curSelectedMesh.getWorldMatrix().getRow(2).toVector3();
                                zAxis.normalize();
                                if(BABYLON.Vector3.Dot(zAxis, cross) < 0.0)
                                    angle = -angle;
                                // Rotate selected mesh
                                let rollQuat: BABYLON.Quaternion = BABYLON.Quaternion.RotationYawPitchRoll(0.0, 0.0, angle);
                                this._curSelectedMesh.rotationQuaternion = this._initialRot.multiply(rollQuat);
                                updateManipulatorsTransforms = true;
                            }
                        }
                        if(updateManipulatorsTransforms)
                        {
                            if(this._meshRotPitch.rotationQuaternion == null)
                                this._meshRotPitch.rotationQuaternion = this._curSelectedMesh.rotationQuaternion.clone();
                            else
                                this._meshRotPitch.rotationQuaternion.copyFrom(this._curSelectedMesh.rotationQuaternion);
                            if(this._meshRotYaw.rotationQuaternion == null)
                                this._meshRotYaw.rotationQuaternion = this._curSelectedMesh.rotationQuaternion.clone();
                            else
                                this._meshRotYaw.rotationQuaternion.copyFrom(this._curSelectedMesh.rotationQuaternion);
                            if(this._meshRotRoll.rotationQuaternion == null)
                                this._meshRotRoll.rotationQuaternion = this._curSelectedMesh.rotationQuaternion.clone();
                            else
                                this._meshRotRoll.rotationQuaternion.copyFrom(this._curSelectedMesh.rotationQuaternion);
                            if(this._meshScaleX.rotationQuaternion == null)
                                this._meshScaleX.rotationQuaternion = this._curSelectedMesh.rotationQuaternion.clone();
                            else
                                this._meshScaleX.rotationQuaternion.copyFrom(this._curSelectedMesh.rotationQuaternion);
                            if(this._meshScaleY.rotationQuaternion == null)
                                this._meshScaleY.rotationQuaternion = this._curSelectedMesh.rotationQuaternion.clone();
                            else
                                this._meshScaleY.rotationQuaternion.copyFrom(this._curSelectedMesh.rotationQuaternion);
                            if(this._meshScaleZ.rotationQuaternion == null)
                                this._meshScaleZ.rotationQuaternion = this._curSelectedMesh.rotationQuaternion.clone();
                            else
                                this._meshScaleZ.rotationQuaternion.copyFrom(this._curSelectedMesh.rotationQuaternion);
                            if(this._meshScaleAll.rotationQuaternion == null)
                                this._meshScaleAll.rotationQuaternion = this._curSelectedMesh.rotationQuaternion.clone();
                            else
                                this._meshScaleAll.rotationQuaternion.copyFrom(this._curSelectedMesh.rotationQuaternion);
                        }
                    }
                    break;
                case ManipulatorMode.Scale:
                    {
                        let curPickRay: BABYLON.Ray = this._scene.createPickingRay(e.clientX, e.clientY, null, this._camera);
                        let curDraggedPickPoint: BABYLON.Vector3 = curPickRay.origin.add(curPickRay.direction.scale(this._pickResult.distance));
                        let curPickOriginToDraggedPickPoint: BABYLON.Vector3 = curDraggedPickPoint.subtract(this._pickResult.pickedPoint);
                        let meshBsphere: BABYLON.BoundingSphere = this._curSelectedMesh.getBoundingInfo().boundingSphere;
                        let realRadius: number = meshBsphere.radius / Math.sqrt(3.0);  // Babylon bsphere radius is incorrect
                        if(this._pickResult.pickedMesh == this._meshScaleX)
                        {
                            // Project onto "right" dir
                            let xAxis: BABYLON.Vector3 = this._curSelectedMesh.getWorldMatrix().getRow(0).toVector3();
                            xAxis.normalize();
                            let dot: number = BABYLON.Vector3.Dot(xAxis, curPickOriginToDraggedPickPoint);
                            let resultingAbsoluteScale: number = dot / realRadius;
                            // Scale selected mesh
                            this._curSelectedMesh.scaling.x = Math.max(0.001, this._initialScale.x + resultingAbsoluteScale);
                            this._meshScaleX.scaling.x = this._curSelectedMesh.scaling.x;
                        }
                        else if(this._pickResult.pickedMesh == this._meshScaleY)
                        {
                            // Project onto "up" dir
                            let yAxis: BABYLON.Vector3 = this._curSelectedMesh.getWorldMatrix().getRow(1).toVector3();
                            yAxis.normalize();
                            let dot: number = BABYLON.Vector3.Dot(yAxis, curPickOriginToDraggedPickPoint);
                            let resultingAbsoluteScale: number = dot / realRadius;
                            // Scale selected mesh
                            this._curSelectedMesh.scaling.y = Math.max(0.001, this._initialScale.y + resultingAbsoluteScale);
                            this._meshScaleY.scaling.y = this._curSelectedMesh.scaling.y;
                        }
                        else if(this._pickResult.pickedMesh == this._meshScaleZ)
                        {
                            // Project onto "at" dir
                            let zAxis: BABYLON.Vector3 = this._curSelectedMesh.getWorldMatrix().getRow(2).toVector3();
                            zAxis.normalize();
                            let dot: number = BABYLON.Vector3.Dot(zAxis, curPickOriginToDraggedPickPoint);
                            let resultingAbsoluteScale: number = dot / realRadius;
                            // Scale selected mesh
                            this._curSelectedMesh.scaling.z = Math.max(0.001, this._initialScale.z + resultingAbsoluteScale);
                            this._meshScaleZ.scaling.z = this._curSelectedMesh.scaling.z;
                        }
                        else if(this._pickResult.pickedMesh == this._meshScaleAll)
                        {
                            // Project onto skew dir (blend of at, up and right)
                            let xAxis: BABYLON.Vector3 = this._curSelectedMesh.getWorldMatrix().getRow(0).toVector3();
                            xAxis.x /= this._curSelectedMesh.scaling.x;
                            let yAxis: BABYLON.Vector3 = this._curSelectedMesh.getWorldMatrix().getRow(1).toVector3();
                            yAxis.y /= this._curSelectedMesh.scaling.y;
                            let zAxis: BABYLON.Vector3 = this._curSelectedMesh.getWorldMatrix().getRow(2).toVector3();
                            zAxis.z /= this._curSelectedMesh.scaling.z;
                            let skewAxis: BABYLON.Vector3 = xAxis;
                            skewAxis.addInPlace(yAxis);
                            skewAxis.addInPlace(zAxis);
                            skewAxis.scaleInPlace(1.0 / 3.0);    // Normalize
                            let dot: number = BABYLON.Vector3.Dot(skewAxis, curPickOriginToDraggedPickPoint);
                            let resultingAbsoluteScale: number = dot / realRadius;
                            // Scale selected mesh
                            this._curSelectedMesh.scaling.x = Math.max(0.001, this._initialScale.x + resultingAbsoluteScale);
                            this._curSelectedMesh.scaling.y = Math.max(0.001, this._initialScale.y + resultingAbsoluteScale);
                            this._curSelectedMesh.scaling.z = Math.max(0.001, this._initialScale.z + resultingAbsoluteScale);
                            this._meshScaleX.scaling.x = this._curSelectedMesh.scaling.x;
                            this._meshScaleY.scaling.y = this._curSelectedMesh.scaling.y;
                            this._meshScaleZ.scaling.z = this._curSelectedMesh.scaling.z;
                        }
                    }
                    break;
            }
        }
    }

    public onPointerUp(e: PointerEvent, p: BABYLON.PickingInfo)
    {
        switch(this._mode)
        {
            case ManipulatorMode.Move:
                if(this._pickResult != null)
                {
                    this._pickResult = null;
                    this._camera.attachControl(this._engine.getRenderingCanvas(), true);
                }
                break;
            case ManipulatorMode.Rotate:
                if(this._pickResult != null)
                {
                    this._pickResult = null;
                    this._camera.attachControl(this._engine.getRenderingCanvas(), true);
                }
                break;
            case ManipulatorMode.Scale:
                if(this._pickResult != null)
                {
                    this._pickResult = null;
                    this._camera.attachControl(this._engine.getRenderingCanvas(), true);
                }
                break;
        }
    }

    public ForceMeshSelection(mesh: BABYLON.Mesh)
    {
        this._forcedMeshSelect = mesh;
        this.SelectObject(mesh);
    }

    public SwitchToMode(mode: ManipulatorMode)
    {
        this.StopMode(this._mode);
        this._mode = mode;
        this.StartMode(this._mode);
    }

    private StartMode(mode: ManipulatorMode)
    {
        switch(mode)
        {
            case ManipulatorMode.Move:
                this._curSelectedMesh.isPickable = true;
                break;
            case ManipulatorMode.Rotate:
                {
                    this._curSelectedMesh.isPickable = false;
                    let meshBsphere: BABYLON.BoundingSphere = this._curSelectedMesh.getBoundingInfo().boundingSphere;
                    let realRadius: number = meshBsphere.radius / Math.sqrt(3.0);  // Babylon bsphere radius is incorrect
                    // Pitch, yaw and roll manipulator
                    this._meshRotPitch.setAbsolutePosition(this._curSelectedMesh.getAbsolutePosition());
                    this._meshRotPitch.isVisible = true;
                    this._meshRotYaw.setAbsolutePosition(this._curSelectedMesh.getAbsolutePosition());
                    this._meshRotYaw.isVisible = true;
                    this._meshRotRoll.setAbsolutePosition(this._curSelectedMesh.getAbsolutePosition());
                    this._meshRotRoll.isVisible = true;
                }
                break;
            case ManipulatorMode.Scale:
                {
                    this._curSelectedMesh.isPickable = false;
                    // Scale X manipulator
                    this._meshScaleX.setAbsolutePosition(this._curSelectedMesh.getAbsolutePosition());
                    this._meshScaleX.isVisible = true;
                    // Scale Y manipulator
                    this._meshScaleY.setAbsolutePosition(this._curSelectedMesh.getAbsolutePosition());
                    this._meshScaleY.isVisible = true;
                    // Scale Z manipulator
                     this._meshScaleZ.setAbsolutePosition(this._curSelectedMesh.getAbsolutePosition());
                    this._meshScaleZ.isVisible = true;
                    // Scale all manipulator
                    this._meshScaleAll.setAbsolutePosition(this._curSelectedMesh.getAbsolutePosition());
                    this._meshScaleAll.isVisible = true;
                }
                break;
        }
    }

    private StopMode(mode: ManipulatorMode)
    {
        switch(mode)
        {
            case ManipulatorMode.Move:
                // Nothing to do
                break;
            case ManipulatorMode.Rotate:
                if(this._meshRotPitch != null)
                    this._meshRotPitch.isVisible = false;
                if(this._meshRotYaw != null)
                    this._meshRotYaw.isVisible = false;
                if(this._meshRotRoll != null)
                    this._meshRotRoll.isVisible = false;
                break;
            case ManipulatorMode.Scale:
                if(this._meshScaleX != null)
                    this._meshScaleX.isVisible = false;
                if(this._meshScaleY != null)
                    this._meshScaleY.isVisible = false;
                if(this._meshScaleZ != null)
                    this._meshScaleZ.isVisible = false;
                if(this._meshScaleAll != null)
                    this._meshScaleAll.isVisible = false;
                break;
        }
    }

    private SelectObject(mesh: BABYLON.Mesh)
    {
        mesh.material = this._selectedMeshMaterial;
        this._curSelectedMesh = mesh;
        // Create manipulator meshes
        let meshBbox: BABYLON.BoundingBox = this._curSelectedMesh.getBoundingInfo().boundingBox;
        let sideMaxExtend: number = Math.max(meshBbox.extendSize.x, meshBbox.extendSize.y);
        let minExtend: number = Math.min(Math.min(meshBbox.extendSize.x, meshBbox.extendSize.y), meshBbox.extendSize.z);
        let pitchYawRollThicknessRatio: number = 0.2;
        // Pitch manipulator
        if(this._meshRotPitch != null)
        {
            this._meshRotPitch.dispose();
            this._meshRotPitch = null;
        }
        this._meshRotPitch = BABYLON.Mesh.CreateTorus("meshRotPitch", sideMaxExtend * 2.0, sideMaxExtend * pitchYawRollThicknessRatio, 32, this._scene, false);
        this._meshRotPitch.setPivotMatrix(BABYLON.Matrix.RotationZ(Math.PI / 2.0));
        this._meshRotPitch.renderingGroupId = 1;
        this._meshRotPitch.material = this._xDirMeshMaterial;
        this._meshRotPitch.isVisible = false;
        // Yaw manipulator
        if(this._meshRotYaw != null)
        {
            this._meshRotYaw.dispose();
            this._meshRotYaw = null;
        }
        this._meshRotYaw = BABYLON.Mesh.CreateTorus("meshRotPitch", sideMaxExtend * 2.0, sideMaxExtend * pitchYawRollThicknessRatio, 32, this._scene, false);
        this._meshRotYaw.renderingGroupId = 1;
        this._meshRotYaw.material = this._yDirMeshMaterial;
        this._meshRotYaw.isVisible = false;
        // Roll manipulator
        if(this._meshRotRoll != null)
        {
            this._meshRotRoll.dispose();
            this._meshRotRoll = null;
        }
        this._meshRotRoll = BABYLON.Mesh.CreateTorus("meshRotRoll", sideMaxExtend * 2.0, sideMaxExtend * pitchYawRollThicknessRatio, 32, this._scene, false);
        this._meshRotRoll.setPivotMatrix(BABYLON.Matrix.RotationX(Math.PI / 2.0));
        this._meshRotRoll.renderingGroupId = 1;
        this._meshRotRoll.material = this._zDirMeshMaterial;
        this._meshRotRoll.isVisible = false;
        // Scale X manipulator
        if(this._meshScaleX != null)
        {
            this._meshScaleX.dispose();
            this._meshScaleX = null;
        }
        let axisTipSizeRatio: number = 0.4;
        let scaleAllSizeRatio: number = 0.5;
        this._meshScaleX = BABYLON.Mesh.CreateCylinder("meshScaleX", meshBbox.extendSize.x, 0, minExtend * axisTipSizeRatio, 32, 1, this._scene, false);
        let pivotTranslate: BABYLON.Matrix = BABYLON.Matrix.Translation(0, -meshBbox.extendSize.x / 2.0, 0);
        let pivotRot: BABYLON.Matrix = BABYLON.Matrix.RotationZ(Math.PI / 2.0);
        this._meshScaleX.setPivotMatrix(pivotTranslate.multiply(pivotRot));
        this._meshScaleX.renderingGroupId = 1;   // To avoid z-buffer test
        this._meshScaleX.material = this._xDirMeshMaterial;
        this._meshScaleX.isVisible = false;
        // Scale Y manipulator
        if(this._meshScaleY != null)
        {
            this._meshScaleY.dispose();
            this._meshScaleY = null;
        }
        this._meshScaleY = BABYLON.Mesh.CreateCylinder("meshScaleY", meshBbox.extendSize.y, 0, minExtend * axisTipSizeRatio, 32, 1, this._scene, false);
        pivotTranslate = BABYLON.Matrix.Translation(0, -meshBbox.extendSize.y / 2.0, 0);
        pivotRot = BABYLON.Matrix.RotationX(Math.PI);
        this._meshScaleY.setPivotMatrix(pivotTranslate.multiply(pivotRot));
        this._meshScaleY.renderingGroupId = 1;
        this._meshScaleY.material = this._yDirMeshMaterial;
        this._meshScaleY.isVisible = false;
        // Scale Z manipulator
        if(this._meshScaleZ != null)
        {
            this._meshScaleZ.dispose();
            this._meshScaleZ = null;
        }
        this._meshScaleZ = BABYLON.Mesh.CreateCylinder("meshScaleZ", meshBbox.extendSize.z, 0, minExtend * axisTipSizeRatio, 32, 1, this._scene, false);
        pivotTranslate = BABYLON.Matrix.Translation(0, -meshBbox.extendSize.z / 2.0, 0);
        pivotRot = BABYLON.Matrix.RotationX(-Math.PI / 2.0);
        this._meshScaleZ.setPivotMatrix(pivotTranslate.multiply(pivotRot));
        this._meshScaleZ.renderingGroupId = 1;
        this._meshScaleZ.material = this._zDirMeshMaterial;
        this._meshScaleZ.isVisible = false;
        // Scale all manipulator
        if(this._meshScaleAll != null)
        {
            this._meshScaleAll.dispose();
            this._meshScaleAll = null;
        }
        this._meshScaleAll = BABYLON.Mesh.CreateSphere("meshScaleAll", 16, minExtend * scaleAllSizeRatio, this._scene, false);
        let manipulatorOffset: BABYLON.Vector3 = this._meshScaleAll.getBoundingInfo().boundingBox.extendSize.scale(-0.5);
        this._meshScaleAll.setPivotMatrix(BABYLON.Matrix.Translation(manipulatorOffset.x, manipulatorOffset.y, manipulatorOffset.z));
        this._meshScaleAll.renderingGroupId = 1;
        this._meshScaleAll.material = this._allDirMeshMaterial;
        this._meshScaleAll.isVisible = false;
    }
}
