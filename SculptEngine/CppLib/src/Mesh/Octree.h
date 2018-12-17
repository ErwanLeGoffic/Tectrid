#ifndef _OCTREE_H_
#define _OCTREE_H_

#include <vector>
#include <memory>
#include "Collisions\BBox.h"

class Mesh;
class BBox;
class OctreeVisitor;

enum CELL_STATE_FLAGS
{
	CELL_STATE_HASTO_UPDATE_SUB_MESH = 1,
	CELL_STATE_HASTO_RECOMPUTE_BBOX = CELL_STATE_HASTO_UPDATE_SUB_MESH << 1,
	CELL_STATE_HASTO_EXTRACT_OUTOFBOUNDS_GEOM = CELL_STATE_HASTO_RECOMPUTE_BBOX << 1
};

class OctreeCell
{
public:
	OctreeCell(BBox const& BBox, OctreeCell* parent): _id(_idGen++), _stateFlags(0), _BBox(BBox), _parent(parent) { AddStateFlags(CELL_STATE_HASTO_UPDATE_SUB_MESH); }
	OctreeCell(OctreeCell const& otherCell, OctreeCell* parent); // For cloning octree

	// Use insert to add tri and vertices initally, but also at update
	void Insert(Mesh const& mesh, std::vector<unsigned int> const& trisToInsert, std::vector<unsigned int> const& vtxToInsert);

	// Visitor's Traverse
	void Traverse(OctreeVisitor& visitor);

	// Vertices and triangles
	std::vector<unsigned int> const& GetTrianglesIdx() const { return _trianglesIdx; }
	std::vector<unsigned int> const& GetVerticesIdx() const { return _verticesIdx; }

	// ID
	unsigned int GetID() const { return _id; }

	// Bbox
	BBox const& GetContentBBox() const { return _contentBBox; }
	BBox& GrabContentBBox() { return _contentBBox; }

	// Flags
	void AddStateFlags(unsigned int flags) { _stateFlags |= flags; }
	void AddStateFlagsUpToRoot(unsigned int flags)
	{
		OctreeCell *cell = this;
		while(cell)
		{
			cell->_stateFlags |= flags;
			cell = cell->_parent;
		}
	}
	void ClearStateFlags(unsigned int flags) { _stateFlags &= ~flags; }
	bool TestStateFlags(unsigned int flags) const { return (_stateFlags & flags) != 0; }

	// Hierarchy
	bool HasChildren();
	std::vector<std::unique_ptr<OctreeCell>> const& GetChildren() const { return _children; }
	bool IsRoot() { return _parent == nullptr; }

	// Rebuild
	void ExtractOutOfBoundsGeom(Mesh const& mesh, std::vector<unsigned int>& extractedTris, std::vector<unsigned int>& extractedVtxs);
	void PurgeEmptyChildren();
	void HandlePendingRemovals(Mesh const& mesh, bool doRemapping);

private:
	std::vector<std::unique_ptr<OctreeCell>> _children;
	OctreeCell* _parent;
	std::vector<unsigned int> _trianglesIdx;
	std::vector<unsigned int> _verticesIdx;
	BBox const _BBox;		// Octree cell bbox
	BBox _contentBBox;		// Bbox encompassing triangles and vertices contained in the cell
	static unsigned int _idGen;
	unsigned int _id;
	unsigned int _stateFlags;
};

#endif // _OCTREE_H_