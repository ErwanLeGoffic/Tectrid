#include "OgsSubMesh.h"

OgsSubMesh::OgsSubMesh(osg::ref_ptr<osg::Geode>& parentNode, SubMesh const* subMesh): _parentOsgNode(parentNode), _subMesh(subMesh)
{
    _ID = _subMesh->GetID();
    _versionNumber = _subMesh->GetVersionNumber();

	_subMeshOsgNode = new osg::Geometry();
	_vertexArray = new osg::Vec3Array();
	_normalArray = new osg::Vec3Array();
	_trianglesArray = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES);
	_subMeshOsgNode->setUseDisplayList(false);
	_subMeshOsgNode->setUseVertexBufferObjects(true);
	RebuildMesh();
	_parentOsgNode->addDrawable(_subMeshOsgNode);
}

OgsSubMesh::~OgsSubMesh()
{
	_parentOsgNode->removeDrawable(_subMeshOsgNode);
}

void OgsSubMesh::Update()
{
    unsigned int curVersionNumber = _subMesh->GetVersionNumber();
    if(_versionNumber != curVersionNumber)  // Only update data if sub mesh has been updated
    {
        _versionNumber = curVersionNumber;
		RebuildMesh();
    }
}

void OgsSubMesh::RebuildMesh()
{
	// Transfer vertices
	std::vector<Vector3> const& vertices = _subMesh->GetVertices();
	_vertexArray->resize(vertices.size());
	memcpy((void*) _vertexArray->getDataPointer(), vertices.data(), vertices.size() * sizeof(Vector3));
	if(_subMeshOsgNode->getVertexArray() == nullptr)
		_subMeshOsgNode->setVertexArray(_vertexArray);
	else
		_vertexArray->dirty();

	// Transfer normals
	std::vector<Vector3> const& normals = _subMesh->GetNormals();
	_normalArray->resize(normals.size());
	memcpy((void*) _normalArray->getDataPointer(), normals.data(), normals.size() * sizeof(Vector3));
	if(_subMeshOsgNode->getNormalArray() == nullptr)
		_subMeshOsgNode->setNormalArray(_normalArray, osg::Array::BIND_PER_VERTEX);
	else
		_normalArray->dirty();

	// Transfer triangles
	std::vector<unsigned int> const& trianglesIDx = _subMesh->GetTriangles();
	_trianglesArray->resize(trianglesIDx.size());
	memcpy((void*) _trianglesArray->getDataPointer(), trianglesIDx.data(), trianglesIDx.size() * sizeof(int));
	if(_subMeshOsgNode->getNumPrimitiveSets() == 0)
		_subMeshOsgNode->addPrimitiveSet(_trianglesArray);
	else
		_trianglesArray->dirty();
}
