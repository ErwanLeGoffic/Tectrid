//#define USE_SUBMESHES
#define USE_FLTK
#define SHOW_WIREFRAME
//#define OCTREE_DEBUG_DRAW
//#define BRUSHES_DEBUG_DRAW
#define CULL_BACKFACE
//#define PLAY_RECORDER
//#define NO_MESH_UPDATE	// To use to profile sculpt engine itself
#define MULTI_SCREEN
//#define DEBUG_HUD

#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/Math>
#include <osg/Material>
#include <osg/ref_ptr>
#include <osg/MatrixTransform>
#include <osg/Depth>
#include <osgFX/Scribe>
#include <memory>
#include <map>
#include <crtdbg.h>
#include "Mesh\GenSphere.h"
#include "Mesh\GenBox.h"
#include "Mesh\GenCylinder.h"
#include "Mesh\GenPyramid.h"
#include "Mesh\Mesh.h"
#include "Mesh\MeshLoader.h"
#include "Mesh\MeshRecorder.h"
#include "Mesh\Octree.h"
#include "Mesh\CSG.h"
#include "Brushes\BrushDraw.h"
#include "Brushes\BrushInflate.h"
#include "Brushes\BrushFlatten.h"
#include "Brushes\BrushDrag.h"
#include "Brushes\BrushDig.h"
#include "Brushes\BrushSmear.h"
#include "Brushes\BrushSmooth.h"
#include "Brushes\BrushCADDrag.h"
#include "SculptEngine.h"

#ifdef USE_SUBMESHES
#include "OgsSubMesh.h"
#endif // USE_SUBMESHES

#ifdef PLAY_RECORDER
#include "Recorder\CommandRecorder.h"
#include <windows.h>
#include <psapi.h>
bool autoPlay = true;
bool playNextFrame = autoPlay;
#endif // PLAY_RECORDER

#ifdef DEBUG_HUD
const float debugHudWidth = 2000.0f;
const float debugHudHeight = 1200.0f;
osg::ref_ptr<osgText::Text> triIdxText;
osg::ref_ptr<osgText::Text> vtx1IdxText;
osg::ref_ptr<osgText::Text> vtx2IdxText;
osg::ref_ptr<osgText::Text> vtx3IdxText;
osg::ref_ptr<osg::Geode> hudGeode;
#endif // DEBUG_HUD

#ifdef USE_FLTK
#pragma warning (disable: 4458)
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#pragma warning (default: 4458)
#include <FL/Fl_Box.H>
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#endif // USE_FLTK

enum SCULPTYPE
{
	SCULPTYPE_DRAW,
	SCULPTYPE_INFLATE,
	SCULPTYPE_FLATTEN,
	SCULPTYPE_DRAG,
	SCULPTYPE_DIG,
	SCULPTYPE_SMEAR,
	SCULPTYPE_SMOOTH,
	SCULPTYPE_CADDRAG
};

osg::ref_ptr<osg::Geode> _geode = nullptr;
#ifdef USE_SUBMESHES
std::map<unsigned int, std::unique_ptr<OgsSubMesh>> _ogsSubMeshes;
#else
osg::ref_ptr<osg::Geometry> _mesh = nullptr;
osg::ref_ptr<osg::Vec3Array> _vertexArray = nullptr;
osg::ref_ptr<osg::Vec3Array> _normalArray = nullptr;
osg::ref_ptr<osg::DrawElementsUInt> _trianglesArray = nullptr;
#endif // !USE_SUBMESHES
osg::ref_ptr<osgViewer::Viewer> _viewer = nullptr;
osg::ref_ptr<osgGA::CameraManipulator> _camManipulator = nullptr;
std::unique_ptr<Mesh> _meshItem = nullptr;
std::unique_ptr<BrushDraw> _brushDraw = nullptr;
std::unique_ptr<BrushInflate> _brushInflate = nullptr;
std::unique_ptr<BrushFlatten> _brushFlatten = nullptr;
std::unique_ptr<BrushDrag> _brushDrag = nullptr;
std::unique_ptr<BrushDig> _brushDig = nullptr;
std::unique_ptr<BrushSmear> _brushSmear = nullptr;
std::unique_ptr<BrushSmooth> _brushSmooth = nullptr;
std::unique_ptr<BrushCADDrag> _brushCADDrag = nullptr;
osg::Vec2 _mousePoint;
osg::Vec2 *_sculptPoint;
bool _sculpting = false;
osg::ref_ptr<osg::Geometry> _uiRingCursor = new osg::Geometry;
osg::ref_ptr<osg::MatrixTransform> _uiRingCursorTransform = new osg::MatrixTransform;
osg::ref_ptr<osg::Group> _sceneGraphRoot = new osg::Group;
#ifdef SHOW_WIREFRAME
osg::ref_ptr<osgFX::Effect> _fxNode = new osgFX::Scribe;
#endif // SHOW_WIREFRAME
float _sculptingStrength = 0.5f;
float _sculptingRadius = 0.12f;
float _modelRadius = 1.0f;
SCULPTYPE _sculptType = SCULPTYPE_DRAW;
bool _useTouch = false;	// Need a toogler as even when recieving touch event we alsoà recieve non touch event, interfering.
#ifdef USE_FLTK
Fl_Button *_buttonUndo;
Fl_Button *_buttonRedo;
void UpdateUndoRedoButtonStates();
#endif // USE_FLTK

void RebuildMesh()
{
#ifdef NO_MESH_UPDATE
	return;
#endif // NO_MESH_UPDATE
#ifdef USE_SUBMESHES
	// Update sub meshes internal structure
	_meshItem->UpdateSubMeshes();
	// Update it to OGS
	unsigned int nbSubMesh = _meshItem->GetSubMeshCount();
	for(unsigned int i = 0; i < nbSubMesh; ++i)
	{
		SubMesh const* submesh = _meshItem->GetSubMesh(i);
		std::map<unsigned int, std::unique_ptr<OgsSubMesh>>::iterator itFindOgsSubMesh = _ogsSubMeshes.find(submesh->GetID());
		if(itFindOgsSubMesh != _ogsSubMeshes.end())
			itFindOgsSubMesh->second->Update();	// Sub mesh found, just update its content
		else
			_ogsSubMeshes[submesh->GetID()].reset(new OgsSubMesh(_geode, submesh));	// Sub mesh doesn't exist, build it
	}
	// Remove submeshes that were deleted
	for(auto it = _ogsSubMeshes.begin(); it != _ogsSubMeshes.end(); /* no increment */)
	{
		if(!_meshItem->IsSubMeshExist(it->first))
			it = _ogsSubMeshes.erase(it);
		else
			++it;
	}
#else
	// Transfer vertices
	std::vector<Vector3> const& vertices = _meshItem->GetVertices();
	_vertexArray->resize(vertices.size());
	memcpy((void*) _vertexArray->getDataPointer(), vertices.data(), vertices.size() * sizeof(Vector3));
	if(_mesh->getVertexArray() == nullptr)
		_mesh->setVertexArray(_vertexArray);
	else
		_vertexArray->dirty();

	// Transfer normals
	std::vector<Vector3> const& normals = _meshItem->GetNormals();
	_normalArray->resize(normals.size());
	memcpy((void*) _normalArray->getDataPointer(), normals.data(), normals.size() * sizeof(Vector3));
	if(_mesh->getNormalArray() == nullptr)
		_mesh->setNormalArray(_normalArray, osg::Array::BIND_PER_VERTEX);
	else
		_normalArray->dirty();

	// Transfer triangles
	std::vector<unsigned int> const& trianglesIDx = _meshItem->GetTriangles();
	_trianglesArray->resize(trianglesIDx.size());
	memcpy((void*) _trianglesArray->getDataPointer(), trianglesIDx.data(), trianglesIDx.size() * sizeof(int));
	if(_mesh->getNumPrimitiveSets() == 0)
		_mesh->addPrimitiveSet(_trianglesArray);
	else
		_trianglesArray->dirty();
#endif // !USE_SUBMESHES
}

Ray GetRayFromScreenpoint(float clientX, float clientY)
{
	osg::Matrix VPW = _viewer->getCamera()->getViewMatrix()
		* _viewer->getCamera()->getProjectionMatrix()
		* _viewer->getCamera()->getViewport()->computeWindowMatrix();

	osg::Matrix invVPW;
	invVPW.invert(VPW);

	osg::Vec3f nearPoint = osg::Vec3f(clientX, clientY, 0.f) * invVPW;
	osg::Vec3f farPoint = osg::Vec3f(clientX, clientY, 1.f) * invVPW;
	osg::Vec3f dir = farPoint - nearPoint;
	dir.normalize();

	return Ray(Vector3(nearPoint.x(), nearPoint.y(), nearPoint.z()), Vector3(dir.x(), dir.y(), dir.z()), FLT_MAX);
}

void CreateUIRingCursor()
{
	// Create wireframe ring (diameter 1.0)
	unsigned int nbSegments = 100;
	float stepAngle = (2.0f * M_PI) / float(nbSegments);
	osg::ref_ptr<osg::Vec3Array> circleVertices = new osg::Vec3Array(nbSegments + 1);
	float curAngle = 0.0f;
	for(unsigned int i = 0; i <= nbSegments; ++i, curAngle += stepAngle)
	{
		(*circleVertices)[i].x() = cosf(curAngle);
		(*circleVertices)[i].z() = sinf(curAngle);
	}
	// Register it to OSG
	_uiRingCursor->setUseDisplayList(false);
	_uiRingCursor->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, circleVertices->size()));
	_uiRingCursor->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
	_uiRingCursor->setVertexArray(circleVertices.get());
	// Make ring cursor always visible
	_uiRingCursor->getOrCreateStateSet()->setAttributeAndModes(new osg::Depth(osg::Depth::ALWAYS), osg::StateAttribute::ON);
	_uiRingCursor->getOrCreateStateSet()->setRenderBinDetails(1000, "RenderBin");
	// Create a transform node and link it to OSG scene
	_uiRingCursorTransform = new osg::MatrixTransform();
	_sceneGraphRoot->addChild(_uiRingCursorTransform);
	_uiRingCursorTransform->addChild(_uiRingCursor);
}

void UpdateOctreeDebugDraw()
{
	static std::vector<osg::ref_ptr<osg::Geometry>> drawableBBoxes;
	static osg::Material *material = nullptr;
	if(material == nullptr)
	{
		material = new osg::Material();
		material->setDiffuse(osg::Material::FRONT, osg::Vec4(1.0, 1.0, 1.0, 1.0));
		material->setSpecular(osg::Material::FRONT, osg::Vec4(1.0, 1.0, 1.0, 1.0));
		material->setShininess(osg::Material::FRONT, 100.0f);
	}
	std::vector<BBox> const& bboxes = _meshItem->GetFragmentsBBox();
	// Clear extra bboxes from scene
	while(drawableBBoxes.size() > bboxes.size())
	{
		_geode->removeDrawable(drawableBBoxes.back().get());
		drawableBBoxes.pop_back();
	}
	// Create or update bboxes
	int curBBox = 0;
	osg::ref_ptr<osg::Vec3Array> wireBoxPt = new osg::Vec3Array(8);
	for(BBox bbox : bboxes)
	{
		bool newBBox = curBBox >= drawableBBoxes.size();

		osg::ref_ptr<osg::Vec3Array> wireBoxVert;
		if(newBBox)
			wireBoxVert = new osg::Vec3Array(24);
		else
			wireBoxVert = (osg::Vec3Array*) drawableBBoxes[curBBox]->getVertexArray();

		(*wireBoxPt)[0] = osg::Vec3(bbox.Min().x, bbox.Min().y, bbox.Min().z);
		(*wireBoxPt)[1] = osg::Vec3(bbox.Max().x, bbox.Min().y, bbox.Min().z);
		(*wireBoxPt)[2] = osg::Vec3(bbox.Max().x, bbox.Max().y, bbox.Min().z);
		(*wireBoxPt)[3] = osg::Vec3(bbox.Min().x, bbox.Max().y, bbox.Min().z);
		(*wireBoxPt)[4] = osg::Vec3(bbox.Min().x, bbox.Min().y, bbox.Max().z);
		(*wireBoxPt)[5] = osg::Vec3(bbox.Max().x, bbox.Min().y, bbox.Max().z);
		(*wireBoxPt)[6] = osg::Vec3(bbox.Max().x, bbox.Max().y, bbox.Max().z);
		(*wireBoxPt)[7] = osg::Vec3(bbox.Min().x, bbox.Max().y, bbox.Max().z);

		(*wireBoxVert)[0] = ((*wireBoxPt)[0]);
		(*wireBoxVert)[1] = ((*wireBoxPt)[1]);

		(*wireBoxVert)[2] = ((*wireBoxPt)[1]);
		(*wireBoxVert)[3] = ((*wireBoxPt)[2]);

		(*wireBoxVert)[4] = ((*wireBoxPt)[2]);
		(*wireBoxVert)[5] = ((*wireBoxPt)[3]);

		(*wireBoxVert)[6] = ((*wireBoxPt)[3]);
		(*wireBoxVert)[7] = ((*wireBoxPt)[0]);

		(*wireBoxVert)[8] = ((*wireBoxPt)[0]);
		(*wireBoxVert)[9] = ((*wireBoxPt)[4]);

		(*wireBoxVert)[10] = ((*wireBoxPt)[1]);
		(*wireBoxVert)[11] = ((*wireBoxPt)[5]);

		(*wireBoxVert)[12] = ((*wireBoxPt)[2]);
		(*wireBoxVert)[13] = ((*wireBoxPt)[6]);

		(*wireBoxVert)[14] = ((*wireBoxPt)[3]);
		(*wireBoxVert)[15] = ((*wireBoxPt)[7]);

		(*wireBoxVert)[16] = ((*wireBoxPt)[4]);
		(*wireBoxVert)[17] = ((*wireBoxPt)[5]);

		(*wireBoxVert)[18] = ((*wireBoxPt)[5]);
		(*wireBoxVert)[19] = ((*wireBoxPt)[6]);

		(*wireBoxVert)[20] = ((*wireBoxPt)[6]);
		(*wireBoxVert)[21] = ((*wireBoxPt)[7]);

		(*wireBoxVert)[22] = ((*wireBoxPt)[7]);
		(*wireBoxVert)[23] = ((*wireBoxPt)[4]);

		if(newBBox)
		{
			drawableBBoxes.push_back(new osg::Geometry);
			drawableBBoxes.back()->setUseDisplayList(false);
			drawableBBoxes.back()->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, wireBoxVert->size()));
			drawableBBoxes.back()->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
			drawableBBoxes.back()->getOrCreateStateSet()->setAttribute(material);
			_geode->addDrawable(drawableBBoxes.back());
			drawableBBoxes[curBBox]->setVertexArray(wireBoxVert.get());
		}
		else
			wireBoxVert->dirty();

		++curBBox;
	}
}

#ifdef BRUSHES_DEBUG_DRAW
void UpdateBrushDebugDraw()
{
#ifdef PLAY_RECORDER
	// Todo: hacky
	_sculptType = SCULPTYPE_DRAG;
#ifdef BRUSHES_DEBUG_DRAW
	if(_brushDrag == nullptr)
		_brushDrag.reset((BrushDrag *) CommandRecorder::GetInstance().GetDragBrush());
#endif // BRUSHES_DEBUG_DRAW
#endif // PLAY_RECORDER
	switch(_sculptType)
	{
	case SCULPTYPE_DRAG:
	{
		// Draw brush projection sphere debug draw
		static osg::ref_ptr<osg::ShapeDrawable> projSphereDrawable;
		static osg::ref_ptr<osg::MatrixTransform> projSphereTransform;
		static osg::Material *projSphereMaterial = nullptr;
		if(projSphereDrawable == nullptr)
		{
			// Create drawable
			projSphereDrawable = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), 1.0f));
			// Create transform and link to scene graph
			projSphereTransform = new osg::MatrixTransform();
			_geode->addChild(projSphereTransform);
			projSphereTransform->addChild(projSphereDrawable);
			// Create material
			projSphereMaterial = new osg::Material();
			projSphereMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.0, 1.0, 0.0, 1.0));
			projSphereMaterial->setAlpha(osg::Material::FRONT, 0.4f);
			// Setup drawable
			projSphereDrawable->setUseDisplayList(false);		
			projSphereDrawable->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
			projSphereDrawable->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
			projSphereDrawable->getOrCreateStateSet()->setAttribute(projSphereMaterial);
			projSphereDrawable->getOrCreateStateSet()->setRenderBinDetails(10000, "RenderBin");
		}
		Vector3 const& projCenter = _brushDrag->GetProjectionCenter();
		osg::Matrix mtx;
		mtx.setTrans(projCenter.x, projCenter.y, projCenter.z);
		mtx.preMultScale(osg::Vec3f(_sculptingRadius, _sculptingRadius, _sculptingRadius));
		projSphereTransform->setMatrix(mtx);

		// Draw drag Ray and intersection point
		static osg::ref_ptr<osg::ShapeDrawable> rayBeginDrawable;
		static osg::ref_ptr<osg::ShapeDrawable> intersectionDrawable;
		static osg::ref_ptr<osg::MatrixTransform> rayBeginTransform;
		static osg::ref_ptr<osg::MatrixTransform> intersectionTransform;
		static osg::Material *rayBeginMaterial = nullptr;
		static osg::Material *intersectionMaterial = nullptr;
		if(rayBeginDrawable == nullptr)
		{
			// Create drawables
			rayBeginDrawable = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), 5.0f));
			intersectionDrawable = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), 5.0f));
			// Create transforms and link to scene graph
			rayBeginTransform = new osg::MatrixTransform();
			_geode->addChild(rayBeginTransform);
			rayBeginTransform->addChild(rayBeginDrawable);
			intersectionTransform = new osg::MatrixTransform();
			_geode->addChild(intersectionTransform);
			intersectionTransform->addChild(intersectionDrawable);
			// Create materials
			rayBeginMaterial = new osg::Material();
			rayBeginMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.0, 0.0, 1.0, 1.0));
			//rayBeginMaterial->setAlpha(osg::Material::FRONT, 0.4f);
			intersectionMaterial = new osg::Material();
			intersectionMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0, 1.0, 0.0, 1.0));
			//intersectionMaterial->setAlpha(osg::Material::FRONT, 0.4f);
			// Setup drawables
			rayBeginDrawable->setUseDisplayList(false);
			rayBeginDrawable->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
			//rayBeginDrawable->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
			rayBeginDrawable->getOrCreateStateSet()->setAttribute(rayBeginMaterial);
			intersectionDrawable->setUseDisplayList(false);
			intersectionDrawable->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
			//intersectionDrawable->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
			intersectionDrawable->getOrCreateStateSet()->setAttribute(intersectionMaterial);
		}
		mtx.makeIdentity();
		Vector3 const& theoricalDestinationPoint = _brushDrag->GetTheoricalDestinationPoint();
		mtx.setTrans(theoricalDestinationPoint.x, theoricalDestinationPoint.y, theoricalDestinationPoint.z);
		rayBeginTransform->setMatrix(mtx);
		Vector3 const& refDraggingPoint = _brushDrag->GetRefDraggedPoint();
		mtx.setTrans(refDraggingPoint.x, refDraggingPoint.y, refDraggingPoint.z);
		intersectionTransform->setMatrix(mtx);
		break;
	}
	}
}
#endif // BRUSHES_DEBUG_DRAW

void StartSculpt(float clientX, float clientY)
{
	switch(_sculptType)
	{
	case SCULPTYPE_DRAW:
		_brushDraw->StartStroke();
		break;
	case SCULPTYPE_INFLATE:
		_brushInflate->StartStroke();
		break;
	case SCULPTYPE_FLATTEN:
		_brushFlatten->StartStroke();
		break;
	case SCULPTYPE_DRAG:
		_brushDrag->StartStroke();
		break;
	case SCULPTYPE_DIG:
		_brushDig->StartStroke();
		break;
	case SCULPTYPE_SMEAR:
		_brushSmear->StartStroke();
		break;
	case SCULPTYPE_SMOOTH:
		_brushSmooth->StartStroke();
		break;
	case SCULPTYPE_CADDRAG:
		_brushCADDrag->StartStroke();
		break;
	}
}

void Sculpt(float clientX, float clientY)
{
	osg::Vec2 curSculptPoint(clientX, clientY);
	Ray ray = GetRayFromScreenpoint(curSculptPoint.x(), curSculptPoint.y());
	switch(_sculptType)
	{
	case SCULPTYPE_DRAW:
		_brushDraw->UpdateStroke(ray, _sculptingRadius, _sculptingStrength);
		break;
	case SCULPTYPE_INFLATE:
		_brushInflate->UpdateStroke(ray, _sculptingRadius, _sculptingStrength);
		break;
	case SCULPTYPE_FLATTEN:
		_brushFlatten->UpdateStroke(ray, _sculptingRadius, _sculptingStrength);
		break;
	case SCULPTYPE_DRAG:
		_brushDrag->UpdateStroke(ray, _sculptingRadius, _sculptingStrength);
		break;
	case SCULPTYPE_DIG:
		_brushDig->UpdateStroke(ray, _sculptingRadius, _sculptingStrength);
		break;
	case SCULPTYPE_SMEAR:
		_brushSmear->UpdateStroke(ray, _sculptingRadius, _sculptingStrength);
		break;
	case SCULPTYPE_SMOOTH:
		_brushSmooth->UpdateStroke(ray, _sculptingRadius, _sculptingStrength);
		break;
	case SCULPTYPE_CADDRAG:
		_brushCADDrag->UpdateStroke(ray, _sculptingRadius, _sculptingStrength);
		break;
	}
	RebuildMesh();
#ifdef OCTREE_DEBUG_DRAW
	UpdateOctreeDebugDraw();
#endif // OCTREE_DEBUG_DRAW
#ifdef BRUSHES_DEBUG_DRAW
	UpdateBrushDebugDraw();
#endif // BRUSHES_DEBUG_DRAW
}

void EndSculpt()
{
	switch(_sculptType)
	{
	case SCULPTYPE_DRAW:
		_brushDraw->EndStroke();
		break;
	case SCULPTYPE_INFLATE:
		_brushInflate->EndStroke();
		break;
	case SCULPTYPE_FLATTEN:
		_brushFlatten->EndStroke();
		break;
	case SCULPTYPE_DRAG:
		_brushDrag->EndStroke();
		break;
	case SCULPTYPE_DIG:
		_brushDig->EndStroke();
		break;
	case SCULPTYPE_SMEAR:
		_brushSmear->EndStroke();
		break;
	case SCULPTYPE_SMOOTH:
		_brushSmooth->EndStroke();
		break;
	case SCULPTYPE_CADDRAG:
		_brushCADDrag->EndStroke();
		break;
	}
	RebuildMesh();
#ifdef OCTREE_DEBUG_DRAW
	UpdateOctreeDebugDraw();
#endif // OCTREE_DEBUG_DRAW
#ifdef BRUSHES_DEBUG_DRAW
	UpdateBrushDebugDraw();
#endif // BRUSHES_DEBUG_DRAW
#ifdef USE_FLTK
	UpdateUndoRedoButtonStates();
#endif // USE_FLTK
}

class myInputEventHandler : public osgGA::GUIEventHandler
{
public:
	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter&);
};

bool myInputEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
#ifdef USE_FLTK
	Fl::check();	// FLTK manual update
#endif // USE_FLTK
	if(ea.getTouchData() != nullptr)
	{
		_mousePoint.set(ea.getX() - ea.getWindowX(), ea.getY() + ea.getWindowY());	// use of GetWindowX/Y as touch event position seems to be screen based
		_useTouch = true;
	}
	else if(!_useTouch)
		_mousePoint.set(ea.getX(), ea.getY());
	switch(ea.getEventType())
	{
#ifndef PLAY_RECORDER
	case(osgGA::GUIEventAdapter::PUSH):
	{
		if(ea.getButton() == 1)	// If left mouse button clicked
		{
			Ray ray = GetRayFromScreenpoint(_mousePoint.x(), _mousePoint.y());
			Vector3 intersection;
			// we pick an object, activate sculpting
			if(_meshItem->GetClosestIntersectionPoint(ray, intersection, nullptr, true))
			{
				_camManipulator = _viewer->getCameraManipulator();
				_viewer->setCameraManipulator(nullptr, false);
				StartSculpt(_mousePoint.x(), _mousePoint.y());
				_sculpting = true;
			}
		}
		break;
	}
	case(osgGA::GUIEventAdapter::DRAG):
	{
		if(_sculpting)
			_sculptPoint = &_mousePoint;
		break;
	}
	case(osgGA::GUIEventAdapter::RELEASE):
	{
		if(_sculpting)
		{
			EndSculpt();
			// Reset manipulator to current camera (because it continues to move when we were sculpting... and no way worked to make it stop)
			if(_camManipulator != nullptr)
			{
				// Reset manipulator
				osg::Vec3d eye;
				osg::Vec3d center;
				osg::Vec3d up;
				_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
				_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up, eye.length());
				_camManipulator->setHomePosition(eye, center, up, false);
				// Set camera manipulator to the viewer
				_viewer->setCameraManipulator(_camManipulator, true);
			}
			_camManipulator = nullptr;
			// Stop sculpting
			_sculpting = false;
			_sculptPoint = nullptr;
		}
		_useTouch = false;
		break;
	}
#endif // !PLAY_RECORDER
#ifdef SHOW_WIREFRAME
	case(osgGA::GUIEventAdapter::KEYDOWN):
	{
		switch(ea.getKey())
		{
		case 'w':
			_fxNode->setEnabled(!_fxNode->getEnabled());
			break;
#ifdef PLAY_RECORDER
		case 'a':
			playNextFrame = true;
			break;
#endif // PLAY_RECORDER
		}
	}
#endif // SHOW_WIREFRAME
	}
	return false;
};

// Per frame update
class NodeUpdateCallback: public osg::NodeCallback
{
public:
	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
	{
#ifdef PLAY_RECORDER
		if(!playNextFrame)
			return;
		static clock_t begin = clock();
		if(CommandRecorder::GetInstance().PlayNextCommand())
		{
#ifdef BRUSHES_DEBUG_DRAW
			UpdateBrushDebugDraw();
#endif // BRUSHES_DEBUG_DRAW
			playNextFrame = autoPlay;
			RebuildMesh();
		}
		else
		{
			static bool resultPrinted = false;
			if(!resultPrinted)
			{
				clock_t end = clock();
				printf("Play time %f\n", double(end - begin) / CLOCKS_PER_SEC);

				HANDLE hProcess = GetCurrentProcess();
				_PROCESS_MEMORY_COUNTERS pmc;
				if(GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
				{
					printf("Cur mem: %lld MB\n", pmc.PagefileUsage / 1024);
					printf("Peak mem: %lld MB\n", pmc.PeakPagefileUsage / 1024);
				}

				resultPrinted = true;
			}
		}
#else
		{
			// Update sculpting
			if(_sculptPoint)
			{
				Sculpt(_sculptPoint->x(), _sculptPoint->y());
				if(_sculptType != SCULPTYPE_DRAG)	// Don't need to move cursor when dragging
					_sculptPoint = nullptr;
			}
			// Update cursor
			Ray ray = GetRayFromScreenpoint(_mousePoint.x(), _mousePoint.y());
			Vector3 intersectionPoint, intersectionNormal;
			if(_meshItem->GetClosestIntersectionPoint(ray, intersectionPoint, &intersectionNormal, true))
			{
				osg::Vec3d at(intersectionNormal.x, intersectionNormal.y, intersectionNormal.z);
				osg::Vec3d up(0.0f, 1.0f, 0.0f);
				if(fabs(up * at) > 0.95f)
					up = osg::Vec3d(1.0f, 0.0f, 0.0f);
				osg::Vec3d left = up ^ at;
				left.normalize();
				up = left ^ at;
				up.normalize();
				osg::Matrix mat(left.x(), left.y(), left.z(), 0.0f,
					at.x(), at.y(), at.z(), 0.0f,
					up.x(), up.y(), up.z(), 0.0f,
					intersectionPoint.x, intersectionPoint.y, intersectionPoint.z, 1.0f);
				mat.preMultScale(osg::Vec3f(_sculptingRadius, _sculptingRadius, _sculptingRadius));
				_uiRingCursorTransform->setMatrix(mat);
			}
			else
			{
				Vector3 uiRingPos = ray.GetOrigin() + (ray.GetDirection() * ray.GetOrigin().Length());
				osg::Vec3d eye;
				osg::Vec3d center;
				osg::Vec3d up;
				_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
				osg::Vec3d at(ray.GetDirection().x, ray.GetDirection().y, ray.GetDirection().z);
				osg::Vec3d left = up ^ at;
				osg::Matrix mat(left.x(), left.y(), left.z(), 0.0f,
					at.x(), at.y(), at.z(), 0.0f,
					up.x(), up.y(), up.z(), 0.0f,
					uiRingPos.x, uiRingPos.y, uiRingPos.z, 1.0f);
				mat.preMultScale(osg::Vec3f(_sculptingRadius, _sculptingRadius, _sculptingRadius));
				_uiRingCursorTransform->setMatrix(mat);
			}
		}
#endif // !PLAY_RECORDER
#ifdef DEBUG_HUD
		{
			Vector3 intersection;
			unsigned int triIdx = 0;
			Ray ray = GetRayFromScreenpoint(_mousePoint.x(), _mousePoint.y());
			if(_meshItem->GetClosestIntersectionPointAndTriangle(ray, intersection, &triIdx, nullptr, true))
			{
				{	// Has to do this to prevent OSG from crashing
					hudGeode->removeDrawable(triIdxText);
					hudGeode->removeDrawable(vtx1IdxText);
					hudGeode->removeDrawable(vtx2IdxText);
					hudGeode->removeDrawable(vtx3IdxText);
					triIdxText = new osgText::Text();
					vtx1IdxText = new osgText::Text();
					vtx2IdxText = new osgText::Text();
					vtx3IdxText = new osgText::Text();
					hudGeode->addDrawable(triIdxText);
					hudGeode->addDrawable(vtx1IdxText);
					hudGeode->addDrawable(vtx2IdxText);
					hudGeode->addDrawable(vtx3IdxText);
				}

				float widthRatio = debugHudWidth / _viewer->getCamera()->getViewport()->width();
				float heightRatio = debugHudHeight / _viewer->getCamera()->getViewport()->height();

				triIdxText->setNodeMask((osg::Node::NodeMask) ~0);
				triIdxText->setText(std::to_string(triIdx));
				triIdxText->setPosition(osg::Vec3(_mousePoint.x() * widthRatio, _mousePoint.y() * heightRatio, 0.0f));

				osg::Matrix VPW = _viewer->getCamera()->getViewMatrix()
					* _viewer->getCamera()->getProjectionMatrix()
					* _viewer->getCamera()->getViewport()->computeWindowMatrix();
				unsigned int const* vtxIdx = &(_meshItem->GetTriangles()[triIdx * 3]);
				Vector3 const& vtx1WorldPoint = _meshItem->GetVertices()[vtxIdx[0]];
				Vector3 const& vtx2WorldPoint = _meshItem->GetVertices()[vtxIdx[1]];
				Vector3 const& vtx3WorldPoint = _meshItem->GetVertices()[vtxIdx[2]];
				osg::Vec3f vtx1ScreenPoint = osg::Vec3f(vtx1WorldPoint.x, vtx1WorldPoint.y, vtx1WorldPoint.z) * VPW;
				vtx1ScreenPoint.x() *= widthRatio;
				vtx1ScreenPoint.y() *= heightRatio;
				osg::Vec3f vtx2ScreenPoint = osg::Vec3f(vtx2WorldPoint.x, vtx2WorldPoint.y, vtx2WorldPoint.z) * VPW;
				vtx2ScreenPoint.x() *= widthRatio;
				vtx2ScreenPoint.y() *= heightRatio;
				osg::Vec3f vtx3ScreenPoint = osg::Vec3f(vtx3WorldPoint.x, vtx3WorldPoint.y, vtx3WorldPoint.z) * VPW;
				vtx3ScreenPoint.x() *= widthRatio;
				vtx3ScreenPoint.y() *= heightRatio;
				vtx1IdxText->setNodeMask((osg::Node::NodeMask) ~0);
				vtx2IdxText->setNodeMask((osg::Node::NodeMask) ~0);
				vtx3IdxText->setNodeMask((osg::Node::NodeMask) ~0);
				vtx1IdxText->setText(std::to_string(vtxIdx[0]));
				vtx2IdxText->setText(std::to_string(vtxIdx[1]));
				vtx3IdxText->setText(std::to_string(vtxIdx[2]));
				vtx1IdxText->setPosition(vtx1ScreenPoint);
				vtx2IdxText->setPosition(vtx2ScreenPoint);
				vtx3IdxText->setPosition(vtx3ScreenPoint);
				vtx1IdxText->update();
				vtx2IdxText->update();
				vtx3IdxText->update();
			}
			else
			{
				triIdxText->setNodeMask((osg::Node::NodeMask) 0);
				vtx1IdxText->setNodeMask((osg::Node::NodeMask) 0);
				vtx2IdxText->setNodeMask((osg::Node::NodeMask) 0);
				vtx3IdxText->setNodeMask((osg::Node::NodeMask) 0);
			}
		}
#endif // DEBUG_HUD
	}
};

#ifdef USE_FLTK
void setSculptStrengthCB(Fl_Widget *o, void *)
{
	Fl_Hor_Value_Slider *slider = (Fl_Hor_Value_Slider *) o;
	_sculptingStrength = float(slider->value()) / 100.0f;
}

void setSculptSizeCB(Fl_Widget *o, void *)
{
	Fl_Hor_Value_Slider *slider = (Fl_Hor_Value_Slider *) o;
	_sculptingRadius = _modelRadius * float(slider->value()) / 100.0f;
}

void setSculptTypeCB(Fl_Button *b, void *)
{
	if(strcmp(b->label(), "Draw") == 0)
		_sculptType = SCULPTYPE_DRAW;
	else if(strcmp(b->label(), "Inflate") == 0)
		_sculptType = SCULPTYPE_INFLATE;
	else if(strcmp(b->label(), "Flatten") == 0)
		_sculptType = SCULPTYPE_FLATTEN;
	else if(strcmp(b->label(), "Drag") == 0)
		_sculptType = SCULPTYPE_DRAG;
	else if(strcmp(b->label(), "Dig") == 0)
		_sculptType = SCULPTYPE_DIG;
	else if(strcmp(b->label(), "Smear") == 0)
		_sculptType = SCULPTYPE_SMEAR;
	else if(strcmp(b->label(), "Smooth") == 0)
		_sculptType = SCULPTYPE_SMOOTH;
	else if(strcmp(b->label(), "CADDrag") == 0)
		_sculptType = SCULPTYPE_CADDRAG;
}

void LoadFileButtonCB(Fl_Button *b, void *)
{
	Fl_File_Chooser *fc = new Fl_File_Chooser(".", "OBJ Files (*.obj)\tSTL Files(*.stl)\tTECTRID Files(*.tct)", Fl_File_Chooser::SINGLE, "Input File");
	fc->show();
	while(fc->shown())
		Fl::wait();
	if(fc->value() != nullptr)
	{
		printf("Load from file: %s\n", fc->value());
		MeshLoader meshLoader;
		_meshItem.reset(meshLoader.LoadFromFile(fc->value()));
		if(!_meshItem->IsManifold())
			fl_alert("Non manifold or closed mesh");
#ifndef PLAY_RECORDER
		_brushDraw.reset(new BrushDraw(*_meshItem));
		_brushInflate.reset(new BrushInflate(*_meshItem));
		_brushFlatten.reset(new BrushFlatten(*_meshItem));
		_brushDrag.reset(new BrushDrag(*_meshItem));
		_brushDig.reset(new BrushDig(*_meshItem));
		_brushSmear.reset(new BrushSmear(*_meshItem));
		_brushSmooth.reset(new BrushSmooth(*_meshItem));
		_brushCADDrag.reset(new BrushCADDrag(*_meshItem));
#endif // !PLAY_RECORDER

		// Get model size (to adjust sculpting radius)
		BBox const& bbox = _meshItem->GetBBox();
		_modelRadius = bbox.Size().Length() * (0.5f / M_SQRT2);
		_sculptingRadius = _modelRadius * 0.12f;

		// Rebuild OSG mesh
		RebuildMesh();

		// Retarget camera
		osg::Vec3d eye;
		osg::Vec3d center;
		osg::Vec3d up;
		_viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
		osg::Vec3d dir = eye - center;
		dir.normalize();
		center = osg::Vec3d(bbox.Center().x, bbox.Center().y, bbox.Center().z);
		eye = center + (dir * _modelRadius * 10.0f);
		_viewer->getCameraManipulator()->setHomePosition(eye, center, up, false);
		_viewer->setCameraManipulator(_viewer->getCameraManipulator(), true);
	}
	delete fc;
}

void SaveFileButtonCB(Fl_Button *b, void *)
{
	Fl_File_Chooser *fc = new Fl_File_Chooser(".", "OBJ Files (*.obj)\tSTL Files(*.stl)\tTECTRID Files(*.tct)", Fl_File_Chooser::CREATE, "Input File");
	fc->show();
	while(fc->shown())
		Fl::wait();
	if(fc->value() != nullptr)
	{
		printf("Save to file: %s\n", fc->value());
		MeshRecorder meshRecorder;
		meshRecorder.SaveToFile(*(_meshItem.get()), fc->value());
	}
	delete fc;
}

void SetMirrorModeCB(Fl_Check_Button *b, void *)
{
	SculptEngine::SetMirrorMode(b->value() != 0);
}

void UndoCB(Fl_Check_Button *b, void *)
{
	_meshItem->Undo();
	UpdateUndoRedoButtonStates();
	RebuildMesh();
#ifdef OCTREE_DEBUG_DRAW
	UpdateOctreeDebugDraw();
#endif // OCTREE_DEBUG_DRAW
#ifdef BRUSHES_DEBUG_DRAW
	UpdateBrushDebugDraw();
#endif // BRUSHES_DEBUG_DRAW
}

void RedoCB(Fl_Check_Button *b, void *)
{
	_meshItem->Redo();
	UpdateUndoRedoButtonStates();
	RebuildMesh();
#ifdef OCTREE_DEBUG_DRAW
	UpdateOctreeDebugDraw();
#endif // OCTREE_DEBUG_DRAW
#ifdef BRUSHES_DEBUG_DRAW
	UpdateBrushDebugDraw();
#endif // BRUSHES_DEBUG_DRAW
}

void CSGTest(Fl_Check_Button *b, void *)
{
	if(!_meshItem->CSGTest())
		fl_alert("CSG failure");
	RebuildMesh();
#ifdef OCTREE_DEBUG_DRAW
	UpdateOctreeDebugDraw();
#endif // OCTREE_DEBUG_DRAW
}

void UpdateUndoRedoButtonStates()
{
	if(_meshItem->CanUndo())
		_buttonUndo->activate();
	else
		_buttonUndo->deactivate();
	if(_meshItem->CanRedo())
		_buttonRedo->activate();
	else
		_buttonRedo->deactivate();
}

#endif // USE_FLTK

#ifdef DEBUG_HUD
osg::Camera* createDebugHUD()
{
	// create a camera to set up the projection and model view matrices, and the subgraph to draw in the HUD
	osg::Camera* camera = new osg::Camera;

	// set the projection matrix
	camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, double(debugHudWidth), 0.0, double(debugHudHeight)));

	// set the view matrix
	camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	camera->setViewMatrix(osg::Matrix::identity());

	// only clear the depth buffer
	camera->setClearMask(GL_DEPTH_BUFFER_BIT);

	// draw subgraph after main camera view.
	camera->setRenderOrder(osg::Camera::POST_RENDER);

	// we don't want the camera to grab event focus from the viewers main camera(s).
	camera->setAllowEventFocus(false);

	// add to this camera a subgraph to render
	hudGeode = new osg::Geode();

	// turn lighting off for the text and disable depth test to ensure it's always ontop.
	osg::StateSet* stateset = hudGeode->getOrCreateStateSet();
	stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

	osg::Vec3 position(debugHudWidth * 0.5f, debugHudHeight * 0.5f, 0.0f);

	triIdxText = new osgText::Text();
	triIdxText->setPosition(position);
	triIdxText->setText("");
	hudGeode->addDrawable(triIdxText);
	
	vtx1IdxText = new osgText::Text();
	vtx1IdxText->setPosition(position);
	vtx1IdxText->setText("");
	hudGeode->addDrawable(vtx1IdxText);

	vtx2IdxText = new osgText::Text();
	vtx2IdxText->setPosition(position);
	vtx2IdxText->setText("");
	hudGeode->addDrawable(vtx2IdxText);

	vtx3IdxText = new osgText::Text();
	vtx3IdxText->setPosition(position);
	vtx3IdxText->setText("");
	hudGeode->addDrawable(vtx3IdxText);

	camera->addChild(hudGeode);
	return camera;
}
#endif // DEBUG_HUD

int main(int, char **)
{
	// Memory leak detection
	_CrtSetBreakAlloc(-1);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// construct the viewer.
	_viewer = new osgViewer::Viewer();
#ifdef MULTI_SCREEN
	_viewer->setUpViewInWindow(450, 30, 800, 600, 1);
#else
	_viewer->setUpViewInWindow(450, 30, 800, 600, 0);
#endif // MULTI_SCREEN

	if(SculptEngine::HasExpired())
		fl_alert("Product has expired on %s. It won't function anymore, please update your version.\n", SculptEngine::GetExpirationDate().c_str());
	// Build sculpting mesh
	/*GenSphere genSphere;
	_meshItem.reset(genSphere.Generate(100.0f));*/
	GenBox genBox;
	_meshItem.reset(genBox.Generate(180.0f, 100.0f, 100.0f));
	//_meshItem.reset(genBox.Generate(100.0f, 100.0f, 100.0f));
	/*GenCylinder genCylinder;
	_meshItem.reset(genCylinder.Generate(180.0f, 25.0f));*/
	/*GenPyramid genPyramid;
	_meshItem.reset(genPyramid.Generate(100.0f, 180.0f, 100.0f));*/
	MeshLoader meshLoader;
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\from turbosquid\\World_Ugliest_Dog_highpoly_obj.OBJ"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\from turbosquid\\ConceptDragon_highpoly.OBJ"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\from turbosquid\\fireBoom.OBJ"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\from turbosquid\\Predator_OBJ.OBJ"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\from 123sculpt\\My Sculpture-1.obj"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\from sculpt3d\\sphere2k.obj"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\From Leopoly\\test flatten 2.obj"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\from turbosquid\\mug.tct"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\from clara.io\\head-scan.obj"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\non closed\\open cube.obj"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\non closed\\FaceTS_OBJ.obj"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\STL\\untitled_bin.stl"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\amas\\test.stl"));
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\From Myminifactory\\girl.obj"));	
	//_meshItem.reset(meshLoader.LoadFromFile("C:\\data\\Nucleus\\Aspi\\part-28570.obj"));
	if(!_meshItem->IsManifold())
#ifdef USE_FLTK
		fl_alert("Non manifold or closed mesh");
#else
		printf("Non manifold or closed mesh");
#endif // USE_FLTK
#ifndef PLAY_RECORDER
	_brushDraw.reset(new BrushDraw(*_meshItem));
	_brushInflate.reset(new BrushInflate(*_meshItem));
	_brushFlatten.reset(new BrushFlatten(*_meshItem));
	_brushDrag.reset(new BrushDrag(*_meshItem));
	_brushDig.reset(new BrushDig(*_meshItem));
	_brushSmear.reset(new BrushSmear(*_meshItem));
	_brushSmooth.reset(new BrushSmooth(*_meshItem));
	_brushCADDrag.reset(new BrushCADDrag(*_meshItem));
#endif // !PLAY_RECORDER
	
	// Get model size (to adjust sculpting radius)
	BBox const& bbox = _meshItem->GetBBox();
	_modelRadius = bbox.Size().Length() * (0.5f / M_SQRT2);
	_sculptingRadius = _modelRadius * 0.12f;

	// Build OSG mesh
	_geode = new osg::Geode();
	osg::Material *material = new osg::Material();
	material->setDiffuse(osg::Material::FRONT, osg::Vec4(1.0, 0.0, 0.0, 1.0));
	material->setSpecular(osg::Material::FRONT, osg::Vec4(1.0, 1.0, 1.0, 1.0));
	material->setShininess(osg::Material::FRONT, 100.0f);
	//material->setAlpha(osg::Material::FRONT, 0.4f);
	_geode->getOrCreateStateSet()->setAttribute(material);
	//_geode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);
#ifdef CULL_BACKFACE
	_geode->getOrCreateStateSet()->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
#endif // CULL_BACKFACE
#ifdef USE_SUBMESHES
	// Sub meshes build
	_meshItem->UpdateSubMeshes();
	unsigned int nbSubMesh = _meshItem->GetSubMeshCount();
	for(unsigned int i = 0; i < nbSubMesh; ++i)
	{
		SubMesh const* submesh = _meshItem->GetSubMesh(i);
		_ogsSubMeshes[submesh->GetID()].reset(new OgsSubMesh(_geode, submesh));
	}
	printf("Submesh count %d\n", (int) nbSubMesh);
#else
	_mesh = new osg::Geometry();
	_vertexArray = new osg::Vec3Array();
	_normalArray = new osg::Vec3Array();
	_trianglesArray = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
	_mesh->setUseDisplayList(false);
	_mesh->setUseVertexBufferObjects(true);
	_mesh->setDataVariance(osg::Object::DYNAMIC);
	RebuildMesh();
	_geode->addDrawable(_mesh);
#endif // !USE_SUBMESHES
#ifdef OCTREE_DEBUG_DRAW
	UpdateOctreeDebugDraw();
#endif // OCTREE_DEBUG_DRAW
#ifndef PLAY_RECORDER
	CreateUIRingCursor();
#endif // !PLAY_RECORDER

	// Frame update callback
	_sceneGraphRoot->setUpdateCallback(new NodeUpdateCallback());

    // add fx node and model to viewer.
#ifdef SHOW_WIREFRAME
	_sceneGraphRoot->addChild(_fxNode);
	_fxNode->addChild(_geode);	
	_fxNode->setEnabled(true);
#else
	_sceneGraphRoot->addChild(_geode);
#endif // !SHOW_WIREFRAME
#ifdef DEBUG_HUD
	_sceneGraphRoot->addChild(createDebugHUD());
#endif // DEBUG_HUD
    _viewer->setSceneData(_sceneGraphRoot);

	// add input handler
	_viewer->addEventHandler(new myInputEventHandler());

	// Turn off near and far clipping
	_viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

	// add the stats handler
	_viewer->addEventHandler(new osgViewer::StatsHandler());

	// add camera manipulator
	_viewer->setCameraManipulator(new osgGA::TrackballManipulator());

	/*osg::Program * vp = new osg::Program();
	// Set per pixel lightning
	SetVirtualProgramShader(vp, "lighting", osg::Shader::VERTEX,
		"Vertex Shader For Per Pixel Lighting",
		PerFragmentLightingVertexShaderSource);

	SetVirtualProgramShader(vp, "lighting", osg::Shader::FRAGMENT,
		"Fragment Shader For Per Pixel Lighting",
		PerFragmentDirectionalLightingFragmentShaderSource);*/

#ifdef USE_FLTK
	Fl_Window window(230, 450);

	// Sculpting strength
	Fl_Hor_Value_Slider sliderStrength(60, 10, 160, 30, "strength");
	sliderStrength.align(FL_ALIGN_LEFT);
	sliderStrength.step(1);
	sliderStrength.precision(0);
	sliderStrength.bounds(0, 100);
	sliderStrength.value(_sculptingStrength * 100.0f);
	sliderStrength.callback(setSculptStrengthCB);

	// Sculpting size
	Fl_Hor_Value_Slider sliderSize(60, 50, 160, 30, "size");
	sliderSize.align(FL_ALIGN_LEFT);
	sliderSize.step(1);
	sliderSize.precision(0);
	sliderSize.bounds(1, 50);
	sliderSize.value(12);
	sliderSize.callback(setSculptSizeCB);

	// Type of sculpting
	Fl_Round_Button radioButtonDraw(10, 90, 80, 20, "Draw");
	radioButtonDraw.type(102);
	radioButtonDraw.value(1);
	radioButtonDraw.down_box(FL_ROUND_DOWN_BOX);
	radioButtonDraw.callback((Fl_Callback*) setSculptTypeCB);
	Fl_Round_Button radioButtonInflate(10, 120, 80, 20, "Inflate");
	radioButtonInflate.type(102);
	radioButtonInflate.value(0);
	radioButtonInflate.down_box(FL_ROUND_DOWN_BOX);
	radioButtonInflate.callback((Fl_Callback*) setSculptTypeCB);
	Fl_Round_Button radioButtonFlatten(10, 150, 80, 20, "Flatten");
	radioButtonFlatten.type(102);
	radioButtonFlatten.value(0);
	radioButtonFlatten.down_box(FL_ROUND_DOWN_BOX);
	radioButtonFlatten.callback((Fl_Callback*) setSculptTypeCB);
	Fl_Round_Button radioButtonDrag(10, 180, 80, 20, "Drag");
	radioButtonDrag.type(102);
	radioButtonDrag.value(0);
	radioButtonDrag.down_box(FL_ROUND_DOWN_BOX);
	radioButtonDrag.callback((Fl_Callback*) setSculptTypeCB);
	Fl_Round_Button radioButtonDig(10, 210, 80, 20, "Dig");
	radioButtonDig.type(102);
	radioButtonDig.value(0);
	radioButtonDig.down_box(FL_ROUND_DOWN_BOX);
	radioButtonDig.callback((Fl_Callback*) setSculptTypeCB);
	Fl_Round_Button radioButtonSmear(10, 240, 80, 20, "Smear");
	radioButtonSmear.type(102);
	radioButtonSmear.value(0);
	radioButtonSmear.down_box(FL_ROUND_DOWN_BOX);
	radioButtonSmear.callback((Fl_Callback*) setSculptTypeCB);
	Fl_Round_Button radioButtonSmooth(10, 270, 80, 20, "Smooth");
	radioButtonSmooth.type(102);
	radioButtonSmooth.value(0);
	radioButtonSmooth.down_box(FL_ROUND_DOWN_BOX);
	radioButtonSmooth.callback((Fl_Callback*) setSculptTypeCB);
	Fl_Round_Button radioButtonCADDrag(10, 300, 80, 20, "CADDrag");
	radioButtonCADDrag.type(102);
	radioButtonCADDrag.value(0);
	radioButtonCADDrag.down_box(FL_ROUND_DOWN_BOX);
	radioButtonCADDrag.callback((Fl_Callback*) setSculptTypeCB);

	// Load/Save
	Fl_Button buttonLoadFile(10, 330, 100, 20, "Load File");
	buttonLoadFile.callback((Fl_Callback*) LoadFileButtonCB);

	Fl_Button buttonSaveFile(120, 330, 100, 20, "Save File");
	buttonSaveFile.callback((Fl_Callback*) SaveFileButtonCB);

	// Mirror mode
	Fl_Check_Button checkboxMirrorMode(10, 360, 80, 20, "Mirror");
	checkboxMirrorMode.value(SculptEngine::IsMirrorModeActivated());
	checkboxMirrorMode.callback((Fl_Callback*) SetMirrorModeCB);

	// Undo/redo
	Fl_Button buttonUndo(10, 390, 100, 20, "Undo");
	_buttonUndo = &buttonUndo;
	buttonUndo.deactivate();
	buttonUndo.callback((Fl_Callback*) UndoCB);

	Fl_Button buttonRedo(120, 390, 100, 20, "Redo");
	_buttonRedo = &buttonRedo;
	buttonRedo.deactivate();
	buttonRedo.callback((Fl_Callback*) RedoCB);

	// CSG
	Fl_Button buttonCSGTest(10, 420, 210, 20, "CSG Test");
	buttonCSGTest.callback((Fl_Callback*) CSGTest);

	window.resizable(&window);
	window.end();
	window.show();
#endif // USE_FLTK	
#ifdef PLAY_RECORDER
	CommandRecorder::GetInstance().Deserialize(*_meshItem);
#endif // PLAY_RECORDER

	return _viewer->run();
}
