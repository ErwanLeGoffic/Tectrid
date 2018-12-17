class BabylonSubMesh
{
    private _babylonsMesh: BABYLON.Mesh;
    private _subMesh: Module.SubMesh;
    private _ID: number;
    private _versionNumber: number;

    public constructor(scene: BABYLON.Scene, parentNode: BABYLON.Node, subMesh: Module.SubMesh, material: BABYLON.StandardMaterial)
    {
        this._subMesh = subMesh;
        this._ID = subMesh.GetID();
        this._versionNumber = subMesh.GetVersionNumber();

        this._babylonsMesh = new BABYLON.Mesh("subMesh-" + this._ID, scene, parentNode);
        this._babylonsMesh.freezeWorldMatrix();
        if(material != null)
        {
            this._babylonsMesh.material = material;
            this._babylonsMesh.material.freeze();
        }        

        let meshData: BABYLON.VertexData = new BABYLON.VertexData();
        let buf: Int8Array = this._subMesh.Triangles();
        let triangles: Uint32Array = new Uint32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
        buf = this._subMesh.Vertices();
        let vertices: Float32Array = new Float32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
        buf = this._subMesh.Normals();
        let normals: Float32Array = new Float32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
        meshData.indices = triangles;
        meshData.positions = vertices;
        meshData.normals = normals;
        this._babylonsMesh.sideOrientation = BABYLON.Mesh.FRONTSIDE;
        this._babylonsMesh.isPickable = false;
        meshData.applyToMesh(this._babylonsMesh, true);
    }

    public Cleanup()
    {
        //this._subMesh.delete();
        this._subMesh = null;
        this._babylonsMesh.dispose();
        this._babylonsMesh = null;
    }

    public Update()
    {
        let curVersionNumber: number = this._subMesh.GetVersionNumber();
        if(this._versionNumber != curVersionNumber)  // Only update data if sub mesh has been updated
        {
            this._versionNumber = curVersionNumber;
            this.UpdataMeshData();
        }
    }

    public UpdataMeshData()
    {
        let buf: Int8Array = this._subMesh.Triangles();
        let triangles: Uint32Array = new Uint32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
        buf = this._subMesh.Vertices();
        let vertices: Float32Array = new Float32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);
        buf = this._subMesh.Normals();
        let normals: Float32Array = new Float32Array(buf.buffer, buf.byteOffset, buf.byteLength / 4);

        // Vertices and normals
        if(this._babylonsMesh._geometry.getTotalVertices() < vertices.length / 3)
        {
            this._babylonsMesh.setVerticesData(BABYLON.VertexBuffer.PositionKind, vertices, true);
            this._babylonsMesh.setVerticesData(BABYLON.VertexBuffer.NormalKind, normals, true);
        }
        else
        {
            this._babylonsMesh.updateVerticesData(BABYLON.VertexBuffer.PositionKind, vertices, false, false);
            this._babylonsMesh.updateVerticesData(BABYLON.VertexBuffer.NormalKind, normals, false, false);
        }

        // Triangle indices
        this._babylonsMesh.setIndices(triangles);
    }

    public GetID(): number
    {
        return this._ID;
    }
}
