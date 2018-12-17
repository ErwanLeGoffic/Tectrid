declare module Module
{
    class BBoxVector
	{
		size(): number;
		get(index: number): BBox;
		delete();
	}

    class SubMesh
    {
        GetID(): number;
        GetVersionNumber(): number;
        Triangles(): Int8Array;
        Vertices(): Int8Array;
        Normals(): Int8Array;
        GetBBox(): BBox;
        delete();
    }

    class Mesh
	{
		Triangles(): Int8Array;
		Vertices(): Int8Array;
		Normals(): Int8Array;
        GetClosestIntersectionPoint(ray: Ray, intersection: Vector3, normal: Vector3, cullBackFace: boolean): boolean;
        GetFragmentsBBox(): BBoxVector;
        GetBBox(): BBox;
        IsManifold(): boolean;
        CanUndo(): boolean;
        Undo(): boolean;
        CanRedo(): boolean;
        Redo(): boolean;
        CSGMerge(otherMesh: Mesh, rotAndScale: Matrix3, position: Vector3, recenterResult: boolean): boolean;
        CSGSubtract(otherMesh: Mesh, rotAndScale: Matrix3, position: Vector3, recenterResult: boolean): boolean;
        CSGIntersect(otherMesh: Mesh, rotAndScale: Matrix3, position: Vector3, recenterResult: boolean): boolean;
        UpdateSubMeshes();
        GetSubMeshCount(): number;
        GetSubMesh(index: number): SubMesh;
        IsSubMeshExist(subMeshID: number): boolean;
        delete();
    }

    class GenSphere
    {
        Generate(radius: number): Mesh;
        delete();
    }

    class GenBox
	{
        Generate(wdith: number, height: number, depth: number): Mesh;
        delete();
	}

    class GenCylinder
    {
        Generate(height: number, radius: number): Mesh;
        delete();
    }

    class GenPyramid
    {
        Generate(wdith: number, height: number, depth: number): Mesh;
        delete();
    }

    class MeshLoader
    {
        LoadFromTextBuffer(fileData: ArrayBuffer, filename: string): Mesh;
        delete();
    }

    class MeshRecorder
    {
        SaveToTextBuffer(mesh: Mesh, fileExt: string): string;
        SetTransformMatrix(transformMatrix: Matrix3);
        delete();
    }

	class Vector3
	{
		constructor(x: number, y: number, z: number);
		X(): number;
		Y(): number;
        Z(): number;
        Length(): number;
        delete();
    }

    class Matrix3
    {
        constructor(right: Vector3, up: Vector3, front: Vector3);
        delete();
    }

	class Ray
	{
        constructor(origin: Vector3, direction: Vector3, length: number);
        delete();
	}

	class BBox
    {
        constructor();
		Min(): Vector3;
        Max(): Vector3;
        Center(): Vector3;
        Size(): Vector3;
        Extents(): Vector3;
        delete();
    }

    class Brush
    {
        StartStroke();
        UpdateStroke(ray: Ray, radius: number, strengthRatio: number);
        EndStroke();
    }

    class BrushDraw extends Brush
    {
        constructor(meshes: Mesh);
        delete();
    }

    class BrushInflate extends Brush
    {
        constructor(meshes: Mesh);
        delete();
    }

    class BrushFlatten extends Brush
    {
        constructor(meshes: Mesh);
        delete();
    }

    class BrushDrag extends Brush
    {
        constructor(meshes: Mesh);
        delete();
    }

    class BrushDig extends Brush
    {
        constructor(meshes: Mesh);
        delete();
    }

    class SculptEngine
    {
        static SetTriangleOrientationInverted(value: boolean);
        static SetMirrorMode(value: boolean);
        static HasExpired(): boolean;
        static GetExpirationDate(): string;
    }
}