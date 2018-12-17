#include "UnityPlugin.h"
#include <algorithm>

#include "Mesh\GenBox.h"
#include "Mesh\GenSphere.h"
#include "Mesh\MeshLoader.h"
#include "Mesh\MeshRecorder.h"
#include "Mesh\Mesh.h"
#include "Mesh\SubMesh.h"
#include "SculptEngine.h"
#include "Brushes\BrushDraw.h"
#include "Brushes\BrushInflate.h"
#include "Brushes\BrushFlatten.h"
#include "Brushes\BrushDrag.h"
#include "Brushes\BrushDig.h"
#include "Brushes\BrushCADDrag.h"

// Octree test
#include "Mesh\Octree.h"

// For openfile dialog 
#include <Windows.h>
#include <Commdlg.h>
#include <tchar.h>
#include <codecvt>
#include <locale>

extern "C"
{
	static MeshLoader loader;
	class CheckHasExpired
	{
	public:
		CheckHasExpired()
		{
			if(SculptEngine_HasExpired())
			{
				std::string expriationDate = SculptEngine::GetExpirationDate();
				std::wstring wExpriationDate;
				wExpriationDate.assign(expriationDate.begin(), expriationDate.end());
				std::wstring expirationMessage = std::wstring(L"Product has expired on ") + wExpriationDate + std::wstring(L". It won't function anymore, please update your version.\n");
				::MessageBox(nullptr, expirationMessage.c_str(), L"Product expired", MB_OK);
			}
		}
	};
	CheckHasExpired checkHasExpired;

	void SculptEngine_SetMirrorMode(bool value)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		SculptEngine::SetMirrorMode(value);
	}

	bool SculptEngine_IsMirrorModeActivated()
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		return SculptEngine::IsMirrorModeActivated();
	}

	bool SculptEngine_HasExpired()
	{
		return SculptEngine::HasExpired();
	}

	char const* SculptEngine_GetExpirationDate()
	{
		static std::string expirationDate = SculptEngine::GetExpirationDate();
		return expirationDate.c_str();
	}

	void* GenBox_Generate(float wdith, float height, float depth)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		static GenBox genBox;
		return genBox.Generate(wdith, height, depth);
	}

	void* GenSphere_Generate(float radius)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		static GenSphere genSphere;
		return genSphere.Generate(radius);
	}

	void* Mesh_Clone(void* mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh == nullptr)
			return nullptr;
		else
			return new Mesh(*typedMesh, false, true);
	}

	void* Mesh_Create(int* triangles, unsigned int triangleCount, float* vertices, unsigned int vertexCount)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		std::vector<unsigned int> triVect(triangles, triangles + triangleCount * 3);
		std::vector<Vector3> vtxVect;
		vtxVect.reserve(vertexCount);
		float *curCoo = vertices;
		float *limitCoo = vertices + vertexCount * 3;
		while(curCoo < limitCoo)
		{
			vtxVect.push_back(Vector3(curCoo[0], curCoo[1], curCoo[2]));
			curCoo += 3;
		}
		return new Mesh(triVect, vtxVect, -1, true, false, false, true, true);
	}

	void* MeshLoader_LoadFromFile(char *filepath)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		if(filepath == nullptr)	// If not file specified, open a file dialog to choose a file
		{
			TCHAR* fileOpenDlgFilePath = new TCHAR[MAX_PATH];
			OPENFILENAME ofn;

			memset(&ofn, 0, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			//ofn.lpstrDefExt = _T("*.obj");
			ofn.lpstrFile = fileOpenDlgFilePath;
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFilter = _T("OBJ file (*.obj)\0 * .obj\0STL file (*.stl)\0 * .stl\0");
			ofn.nFilterIndex = 0;
			//ofn.lpstrInitialDir = this->InitialDir;
			ofn.lpstrTitle = _T("Choose a file to load");
			ofn.Flags = OFN_NOCHANGEDIR;

			if((GetOpenFileName(&ofn) != 0) && (_tcslen(fileOpenDlgFilePath) != 0))
			{
				using convert_type = std::codecvt_utf8<wchar_t>;
				std::wstring_convert<convert_type, wchar_t> converter;
				return loader.LoadFromFile(converter.to_bytes(fileOpenDlgFilePath));
			}
		}
		else
			return loader.LoadFromFile(std::string(filepath));
		return nullptr;
	}
	
	void* MeshLoader_LoadFromTextBuffer(char *fileData, char *filename)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		return loader.LoadFromTextBuffer(std::string(fileData), std::string(filename));
	}

	bool MeshRecorder_Save(char *filepath, void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh == nullptr)
			return false;
		static MeshRecorder recorder;
		if(filepath == nullptr)	// If not file specified, open a file dialog to choose a file to save to
		{
			TCHAR* fileSaveDlgFilePath = new TCHAR[MAX_PATH];
			OPENFILENAME ofn;

			memset(&ofn, 0, sizeof(ofn));

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			//ofn.lpstrDefExt = _T("*.obj");
			ofn.lpstrFile = fileSaveDlgFilePath;
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrFilter = _T("OBJ file (*.obj)\0 * .obj\0STL file (*.stl)\0 * .stl\0");
			ofn.nFilterIndex = 0;
			//ofn.lpstrInitialDir = this->InitialDir;
			ofn.lpstrTitle = _T("Choose a file to save to");
			ofn.Flags = OFN_NOCHANGEDIR;

			GetSaveFileName(&ofn);
			if(_tcslen(fileSaveDlgFilePath) != 0)
			{
				using convert_type = std::codecvt_utf8<wchar_t>;
				std::wstring_convert<convert_type, wchar_t> converter;
				return recorder.SaveToFile(*typedMesh, converter.to_bytes(fileSaveDlgFilePath));
			}
		}
		else
			return recorder.SaveToFile(*typedMesh, std::string(filepath));
		return false;
	}

	void Mesh_Delete(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		delete typedMesh;
	}

	bool Mesh_IsManifold(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return typedMesh->IsManifold();
		return false;
	}

	unsigned int Mesh_GetID(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return typedMesh->GetID();
		return (unsigned int) ~0;
	}

	void Mesh_GetBBox(void *mesh, float *min, float *max)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
		{
			BBox const& bbox = typedMesh->GetBBox();
			ASSERT(bbox.IsValid());
			min[0] = bbox.Min().x;
			min[1] = bbox.Min().y;
			min[2] = bbox.Min().z;
			max[0] = bbox.Max().x;
			max[1] = bbox.Max().y;
			max[2] = bbox.Max().z;
		}
	}

	unsigned int Mesh_GetNbTriangles(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return (unsigned int) typedMesh->GetTriangles().size();
		return 0;
	}
	
	unsigned int Mesh_GetNbVertices(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return (unsigned int) typedMesh->GetVertices().size();
		return 0;
	}

	void Mesh_FillTriangles(void *mesh, int* data)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			memcpy(data, typedMesh->GetTriangles().data(), typedMesh->GetTriangles().size() * sizeof(unsigned int));
	}

	void Mesh_FillVertices(void *mesh, float* data)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			memcpy(data, typedMesh->GetVertices().data(), typedMesh->GetVertices().size() * 3 * sizeof(float));
	}

	void Mesh_FillNormals(void *mesh, float* data)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			memcpy(data, typedMesh->GetNormals().data(), typedMesh->GetNormals().size() * 3 * sizeof(float));
	}

	bool Mesh_GetClosestIntersectionPoint(void *mesh, float* meshRotAndScale3x3Matrix, float* meshPosition, float* ray, float* intersectionPoint, float* intersectionNormal)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
		{
			// Create world to local and local to world transforms
			Matrix3 mtx(meshRotAndScale3x3Matrix);
			Matrix3 invMtx(mtx);
			invMtx.Invert();
			Vector3 pos(meshPosition[0], meshPosition[1], meshPosition[2]);
			// Create our ray in local mesh coordinates
			Ray typedRay(Vector3(ray[0], ray[1], ray[2]), Vector3(ray[3], ray[4], ray[5]), FLT_MAX);
			typedRay.GrabOrigin() -= pos;
			invMtx.Transform(typedRay.GrabOrigin());
			invMtx.Transform(typedRay.GrabDirection());
			typedRay.GrabDirection().Normalize();
			// Get closest intersection
			Vector3 typedPoint, typedNormal;
			bool result = typedMesh->GetClosestIntersectionPoint(typedRay, typedPoint, &typedNormal, true);
			// Transform intersection point from local to world and set it on output
			mtx.Transform(typedPoint);
			typedPoint += pos;
			intersectionPoint[0] = typedPoint.x;
			intersectionPoint[1] = typedPoint.y;
			intersectionPoint[2] = typedPoint.z;
			if(intersectionNormal != nullptr)
			{
				// Transform intersection normal from local to world and set it on output
				mtx.Transform(typedNormal);
				intersectionNormal[0] = typedNormal.x;
				intersectionNormal[1] = typedNormal.y;
				intersectionNormal[2] = typedNormal.z;
			}
			return result;
		}
		return false;
	}

	bool Mesh_CanUndo(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return typedMesh->CanUndo();
		return false;
	}

	bool Mesh_Undo(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return typedMesh->Undo();
		return false;
	}

	bool Mesh_CanRedo(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return typedMesh->CanRedo();
		return false;
	}

	bool Mesh_Redo(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return typedMesh->Redo();
		return false;
	}

	void* BrushDraw_Create(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return new BrushDraw(*typedMesh);
		return nullptr;
	}

	void* BrushInflate_Create(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return new BrushInflate(*typedMesh);
		return nullptr;
	}

	void* BrushFlatten_Create(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return new BrushFlatten(*typedMesh);
		return nullptr;
	}

	void* BrushDrag_Create(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return new BrushDrag(*typedMesh);
		return nullptr;
	}

	void* BrushDig_Create(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return new BrushDig(*typedMesh);
		return nullptr;
	}

	void* BrushCADDrag_Create(void *mesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		if(typedMesh != nullptr)
			return new BrushCADDrag(*typedMesh);
		return nullptr;
	}

	void Brush_StartStroke(void *brush)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Brush* typedBrush = (Brush*) brush;
		if(typedBrush != nullptr)
			typedBrush->StartStroke();
	}

	void Brush_UpdateStroke(void *brush, float* meshRotAndScale3x3Matrix, float* meshPosition, float* rayOrigin, float* rayDirection, float rayLength, float radius, float effectRatio)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Brush* typedBrush = (Brush*) brush;
		if(typedBrush != nullptr)
		{
			Ray ray(Vector3(rayOrigin[0], rayOrigin[1], rayOrigin[2]), Vector3(rayDirection[0], rayDirection[1], rayDirection[2]), rayLength);
			Vector3 pos(meshPosition[0], meshPosition[1], meshPosition[2]);
			Matrix3 invMtx(meshRotAndScale3x3Matrix);
			invMtx.Invert();
			ray.GrabOrigin() -= pos;
			invMtx.Transform(ray.GrabOrigin());
			invMtx.Transform(ray.GrabDirection());
			ray.GrabDirection().Normalize();
			Vector3 radiusVect(radius, radius, radius);
			invMtx.Transform(radiusVect);
			radius = radiusVect.Length();	// This way, object scale is applied on the radius
			typedBrush->UpdateStroke(ray, radius, effectRatio);
		}
	}

	void Brush_EndStroke(void *brush)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Brush* typedBrush = (Brush*) brush;
		if(typedBrush != nullptr)
			typedBrush->EndStroke();
	}

	void Brush_Delete(void *brush)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Brush* typedBrush = (Brush*) brush;
		delete typedBrush;
	}

	// Sub meshes
	void Mesh_UpdateSubMeshes(void *fullMesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedFullMesh = (Mesh*) fullMesh;
		if(typedFullMesh != nullptr)
			typedFullMesh->UpdateSubMeshes();
	}

	unsigned int Mesh_GetSubMeshCount(void *fullMesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedFullMesh = (Mesh*) fullMesh;
		if(typedFullMesh != nullptr)
			return typedFullMesh->GetSubMeshCount();
		return 0;
	}

	void* Mesh_GetSubMesh(void *fullMesh, unsigned int index)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedFullMesh = (Mesh*) fullMesh;
		if(typedFullMesh != nullptr)
			return (void *) typedFullMesh->GetSubMesh(index);
		return nullptr;
	}

	bool Mesh_IsSubMeshExist(void *fullMesh, unsigned int submeshID)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedFullMesh = (Mesh*) fullMesh;
		if(typedFullMesh != nullptr)
			return typedFullMesh->IsSubMeshExist(submeshID);
		return false;
	}

	unsigned int SubMesh_GetID(void *subMesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		SubMesh* typedSubMesh = (SubMesh*) subMesh;
		if(typedSubMesh != nullptr)
			return typedSubMesh->GetID();
		return (unsigned int) ~0;
	}

	unsigned int SubMesh_GetVersionNumber(void *subMesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		SubMesh* typedSubMesh = (SubMesh*) subMesh;
		if(typedSubMesh != nullptr)
			return typedSubMesh->GetVersionNumber();
		return 0;
	}

	void SubMesh_GetBBox(void *subMesh, float *min, float *max)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		SubMesh* typedSubMesh = (SubMesh*) subMesh;
		if(typedSubMesh != nullptr)
		{
			BBox const& bbox = typedSubMesh->GetBBox();
			ASSERT(bbox.IsValid());
			min[0] = bbox.Min().x;
			min[1] = bbox.Min().y;
			min[2] = bbox.Min().z;
			max[0] = bbox.Max().x;
			max[1] = bbox.Max().y;
			max[2] = bbox.Max().z;
		}
	}

	unsigned int SubMesh_GetNbTriangles(void *subMesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		SubMesh* typedSubMesh = (SubMesh*) subMesh;
		if(typedSubMesh != nullptr)
			return (unsigned int) typedSubMesh->GetTriangles().size();
		return 0;
	}

	unsigned int SubMesh_GetNbVertices(void *subMesh)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		SubMesh* typedSubMesh = (SubMesh*) subMesh;
		if(typedSubMesh != nullptr)
			return (unsigned int) typedSubMesh->GetVertices().size();
		return 0;
	}

	void SubMesh_FillTriangles(void *subMesh, int* data)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		SubMesh* typedSubMesh = (SubMesh*) subMesh;
		if(typedSubMesh != nullptr)
			memcpy(data, typedSubMesh->GetTriangles().data(), typedSubMesh->GetTriangles().size() * sizeof(unsigned int));
	}

	void SubMesh_FillVertices(void *subMesh, float* data)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		SubMesh* typedSubMesh = (SubMesh*) subMesh;
		if(typedSubMesh != nullptr)
			memcpy(data, typedSubMesh->GetVertices().data(), typedSubMesh->GetVertices().size() * 3 * sizeof(float));
	}

	void SubMesh_FillNormals(void *subMesh, float* data)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		SubMesh* typedSubMesh = (SubMesh*) subMesh;
		if(typedSubMesh != nullptr)
			memcpy(data, typedSubMesh->GetNormals().data(), typedSubMesh->GetNormals().size() * 3 * sizeof(float));
	}

	bool CSGMerge(void *mesh, void *otherMesh, float* rotAndScale3x3Matrix, float* position)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		Mesh* typedOtherMesh = (Mesh*) otherMesh;
		if((typedMesh != nullptr) && (typedOtherMesh))
		{
			Matrix3 mtx(rotAndScale3x3Matrix);
			Vector3 pos(position[0], position[1], position[2]);
			return typedMesh->CSGMerge(*typedOtherMesh, mtx, pos, false);
		}
		return false;
	}

	bool CSGSubtract(void *mesh, void *otherMesh, float* rotAndScale3x3Matrix, float* position)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		Mesh* typedOtherMesh = (Mesh*) otherMesh;
		if((typedMesh != nullptr) && (typedOtherMesh))
		{
			Matrix3 mtx(rotAndScale3x3Matrix);
			Vector3 pos(position[0], position[1], position[2]);
			return typedMesh->CSGSubtract(*typedOtherMesh, mtx, pos, false);
		}
		return false;
	}

	bool CSGIntersect(void *mesh, void *otherMesh, float* rotAndScale3x3Matrix, float* position)
	{
#ifdef _DEBUG
		_control87(MCW_EM, MCW_EM); // Turn off FPU exception (needed in debug build not to crash unity)
#endif	// _DEBUG
		Mesh* typedMesh = (Mesh*) mesh;
		Mesh* typedOtherMesh = (Mesh*) otherMesh;
		if((typedMesh != nullptr) && (typedOtherMesh))
		{
			Matrix3 mtx(rotAndScale3x3Matrix);
			Vector3 pos(position[0], position[1], position[2]);
			return typedMesh->CSGIntersect(*typedOtherMesh, mtx, pos, false);
		}
		return false;
	}
}