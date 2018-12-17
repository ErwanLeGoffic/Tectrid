#ifndef _OCTREE_VISITOR_BUILDANDCOLLECTSUBMESHES_H_
#define _OCTREE_VISITOR_BUILDANDCOLLECTSUBMESHES_H_

#include <vector>
#include <map>
#include <set>
#include <memory>
#include "OctreeVisitor.h"

class Mesh;
class SubMesh;

// VisitorBuildAndCollectSubMeshes is for the moment only used to inject a split mesh into unity(due to its limit at 64K vertices per mesh)
class VisitorBuildAndCollectSubMeshes : public OctreeVisitor
{
public:
	VisitorBuildAndCollectSubMeshes(Mesh& fullMesh): OctreeVisitor(), _fullMesh(fullMesh) {}
	VisitorBuildAndCollectSubMeshes(Mesh& fullMesh, VisitorBuildAndCollectSubMeshes const& cloneFrom);
	
	virtual bool HasToVisit(OctreeCell& cell);
	virtual void VisitEnter(OctreeCell& cell);
	virtual void VisitLeave(OctreeCell& cell);
	
	unsigned int GetSubMeshCount() const { return (unsigned int) _subMeshesIds.size();  }
	SubMesh const* GetSubMesh(unsigned int index) const { return _subMeshes.at(_subMeshesIds[index]).get(); }
	SubMesh* GrabSubMesh(unsigned int index) { return _subMeshes.at(_subMeshesIds[index]).get(); }
	bool IsSubMeshExist(unsigned int subMeshID) const { return _subMeshes.find(subMeshID) != _subMeshes.end(); }

private:
	Mesh& _fullMesh;
	std::map<unsigned int, std::shared_ptr<SubMesh>> _subMeshes;
	std::vector<unsigned int> _subMeshesIds;
	std::set<unsigned int> _subMeshesToDelete;	// Will store the mesh that are detected as deleted, to be able to clean our data in "EndTraverse()"
};

#endif // _OCTREE_VISITOR_BUILDANDCOLLECTSUBMESHES_H_