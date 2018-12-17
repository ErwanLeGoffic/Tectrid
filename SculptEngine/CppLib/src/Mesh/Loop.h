#ifndef _LOOPS_H_
#define _LOOPS_H_

#include <memory>
#include <vector>
#include <map>
#include "Collisions\Ray.h"

class Mesh;
class BSphere;
class BBox;

class LoopElement
{
public:
	LoopElement(unsigned int vtxId): _prev(nullptr), _vtxId(vtxId), _next(nullptr) { }
	~LoopElement()
	{
		ASSERT((_next == nullptr) || (_vtxId != _next->_vtxId));
		if(_prev != nullptr)
			_prev->_next = nullptr;
		if(_next != nullptr)
			_next->_prev = nullptr;
		delete _next;
	}

	LoopElement const* GetPrev() const { return _prev; }
	void SetPrev(LoopElement* prev, Mesh const& _mesh);

	unsigned int GetVtxId() const { return _vtxId; }

	LoopElement const* GetNext() const { return _next; }
	LoopElement* GrabNext() { return _next; }
	void SetNext(LoopElement* next, Mesh const& _mesh);

	void ReverserLinking(LoopElement *source);	// Recursively switch next and prev

	Ray const& GetRayToPrev() const { return _rayToPrev; }
	Ray const& GetRayToNext() const { return _rayToNext; }

private:
	void ComputeRay(Vector3 const& a, Vector3 const& b, Ray& outRay);

	LoopElement* _prev;
	Ray _rayToPrev;	// Ray from the vertex to prev vertex	// Todo: try to remove it (and keep _rayToNext only)
	unsigned int _vtxId;
	Ray _rayToNext;	// Ray from the vertex to next vertex
	LoopElement* _next;
};

class Loop
{
public:
	Loop(LoopElement* root): _root(root) { }

	LoopElement const* GetRoot() const { return _root.get(); }

	bool Intersects(BSphere const& sphere) const;
	bool Intersects(BBox const& box) const;

	unsigned int CountElements() const;

	bool IsEmpty() const { return _root == nullptr; }
	bool IsClosed() const;

	bool IsClockwize(Vector3 const& loopCenter, Vector3 const& loopCenterNormal, std::vector<Vector3> const& vertices);

private:
	std::unique_ptr<LoopElement> _root;
};

class LoopBuilder
{
public:
	LoopBuilder(Mesh const& mesh): _mesh(mesh) {}

	bool RegisterEdge(unsigned int idVtx1, unsigned int idVtx2);
	void BuildLoops(std::vector<Loop>& outLoops);

private:
	std::map<unsigned int, LoopElement*> _registeredLoopElements;	// associate vtx ID to the loop element
	Mesh const& _mesh;
};

#endif // _LOOPS_H_