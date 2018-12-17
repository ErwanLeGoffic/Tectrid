#ifndef _OGS_SUBMESH_H_
#define _OGS_SUBMESH_H_

#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Geometry>
#include <osg/Math>
#include <osg/ref_ptr>
#include "Mesh/SubMesh.h"

class OgsSubMesh
{
public:
	OgsSubMesh(osg::ref_ptr<osg::Geode>& parentNode, SubMesh const* subMesh);
	~OgsSubMesh();
	void Update();
	unsigned int getID() const { return _ID; }

private:
	void RebuildMesh();

	osg::ref_ptr<osg::Vec3Array> _vertexArray = nullptr;
	osg::ref_ptr<osg::Vec3Array> _normalArray = nullptr;
	osg::ref_ptr<osg::DrawElementsUInt> _trianglesArray = nullptr;
	osg::ref_ptr<osg::Geometry> _subMeshOsgNode = nullptr;
	osg::ref_ptr<osg::Geode> _parentOsgNode = nullptr;
	SubMesh const* _subMesh;
	unsigned int _ID;
	unsigned int _versionNumber;
};

#endif // _OGS_SUBMESH_H_