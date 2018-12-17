#include "Loop.h"
#include "SculptEngine.h"
#include "Math\Math.h"
#include "Collisions\BBox.h"
#include "Collisions\BSphere.h"
#include "Mesh.h"

void LoopElement::SetPrev(LoopElement* prev, Mesh const& _mesh)
{
	prev->_next = this;
	ComputeRay(_mesh.GetVertices()[prev->_vtxId], _mesh.GetVertices()[_vtxId], prev->_rayToNext);
	this->_prev = prev;
	ComputeRay(_mesh.GetVertices()[_vtxId], _mesh.GetVertices()[_prev->_vtxId], _rayToPrev);
}

void LoopElement::SetNext(LoopElement* next, Mesh const& _mesh)
{
	next->_prev = this;
	ComputeRay(_mesh.GetVertices()[next->_vtxId], _mesh.GetVertices()[_vtxId], next->_rayToPrev);
	this->_next = next;
	ComputeRay(_mesh.GetVertices()[_vtxId], _mesh.GetVertices()[_next->_vtxId], _rayToNext);
}

void LoopElement::ComputeRay(Vector3 const& a, Vector3 const& b, Ray& outRay)
{
	Vector3 dir = a - b;
	float length = dir.Length();
	if(length != 0.0f)
		dir /= length;
	outRay.SetOrigin(a);
	outRay.SetDirection(dir);
	outRay.SetLength(length);
}

void LoopElement::ReverserLinking(LoopElement *source)
{
	Swap<LoopElement*>(_next, _prev);
	Swap<Ray>(_rayToNext, _rayToPrev);
	if(_prev && (_prev != source))
		_prev->ReverserLinking(this);
	if(_next && (_next != source))
		_next->ReverserLinking(this);
}

bool Loop::Intersects(BSphere const& sphere) const
{
	LoopElement const* curVtx = _root.get();
	do
	{
		if(curVtx->GetRayToNext().Intersects(sphere))
			return true;
		curVtx = curVtx->GetNext();
	} while(curVtx != _root.get());
	return false;
}

bool Loop::Intersects(BBox const& box) const
{
	LoopElement const* curVtx = _root.get();
	do
	{
		if(curVtx->GetRayToNext().Intersects(box))
			return true;
		curVtx = curVtx->GetNext();
	} while(curVtx != _root.get());
	return false;
}

unsigned int Loop::CountElements() const
{
	unsigned int count = 0;
	LoopElement const* curVtx = _root.get();
	do
	{
		++count;
		curVtx = curVtx->GetNext();
	} while(curVtx != _root.get());
	return count;
}

bool Loop::IsClosed() const
{
	if(IsEmpty())
		return false;
	LoopElement const* curVtx = _root.get();
	do
	{
		curVtx = curVtx->GetNext();
		if(curVtx == nullptr)
			return false;
	} while(curVtx != _root.get());
	return true;
}

bool Loop::IsClockwize(Vector3 const& loopCenter, Vector3 const& loopCenterNormal, std::vector<Vector3> const& vertices)
{
	float cumulAngle = 0.0f;
	LoopElement const* curVtx = _root.get();
	Vector3 a = (vertices[curVtx->GetVtxId()] - loopCenter).Normalized();
	do
	{
		Vector3 b = (vertices[curVtx->GetNext()->GetVtxId()] - loopCenter).Normalized();
		float angle = safe_acosf(a.Dot(b));
		if(a.Cross(b).Dot(loopCenterNormal) < 0.0f)
			angle = -angle;
		cumulAngle += angle;
		curVtx = curVtx->GetNext();
		a = b;
		if(curVtx == nullptr)
			return false;
	} while(curVtx != _root.get());
	return cumulAngle > 0.0f;
}

bool LoopBuilder::RegisterEdge(unsigned int idVtx1, unsigned int idVtx2)
{
	// Link new edge to the existing ones
	LoopElement* vtx1LoopElement = nullptr;
	LoopElement* vtx2LoopElement = nullptr;
	for(auto& pair : _registeredLoopElements)
	{
		if(pair.first == idVtx1)
		{
			ASSERT(vtx1LoopElement == nullptr);
			vtx1LoopElement = pair.second;
			ASSERT(pair.first == pair.second->GetVtxId());
		}
		if(pair.first == idVtx2)
		{
			ASSERT(vtx2LoopElement == nullptr);
			vtx2LoopElement = pair.second;
			ASSERT(pair.first == pair.second->GetVtxId());
		}
	}
	if((vtx1LoopElement != nullptr) && (vtx2LoopElement != nullptr))
	{
		if(vtx1LoopElement->GetNext())
		{
			ASSERT(vtx1LoopElement->GetPrev() == nullptr);
			if(vtx1LoopElement->GetPrev() != nullptr)
				return false;	// The loops might be build from non manifold data, reject this edge
			if(vtx2LoopElement->GetNext() != nullptr)
			{
				ASSERT(vtx2LoopElement->GetPrev() == nullptr);
				if(vtx2LoopElement->GetPrev() != nullptr)
					return false;	// Maybe an error in the loop reversing ?
				vtx2LoopElement->ReverserLinking(nullptr);
			}
			ASSERT(vtx2LoopElement->GetNext() == nullptr);
			vtx2LoopElement->SetNext(vtx1LoopElement, _mesh);
		}
		else
		{
			ASSERT(vtx1LoopElement->GetNext() == nullptr);
			if(vtx1LoopElement->GetNext() != nullptr)
				return false;	// The loops might be build from non manifold data, reject this edge
			if(vtx2LoopElement->GetPrev() != nullptr)
			{
				ASSERT(vtx2LoopElement->GetNext() == nullptr);
				if(vtx2LoopElement->GetNext() != nullptr)
					return false;	// The loops might be build from non manifold data, reject this edge
				vtx2LoopElement->ReverserLinking(nullptr);
			}
			ASSERT(vtx2LoopElement->GetPrev() == nullptr);
			vtx2LoopElement->SetPrev(vtx1LoopElement, _mesh);
		}
	}
	else if(vtx1LoopElement != nullptr)
	{
		ASSERT(vtx2LoopElement == nullptr);
		if(vtx1LoopElement->GetNext() != nullptr)
		{
			ASSERT(vtx1LoopElement->GetPrev() == nullptr);
			if(vtx1LoopElement->GetPrev() != nullptr)
				return false;	// The loops might be build from non manifold data, reject this edge
			vtx2LoopElement = new LoopElement(idVtx2);
			_registeredLoopElements[idVtx2] = vtx2LoopElement;
			vtx1LoopElement->SetPrev(vtx2LoopElement, _mesh);
		}
		else
		{
			ASSERT(vtx1LoopElement->GetNext() == nullptr);
			if(vtx1LoopElement->GetNext() != nullptr)
				return false;	// The loops might be build from non manifold data, reject this edge
			vtx2LoopElement = new LoopElement(idVtx2);
			_registeredLoopElements[idVtx2] = vtx2LoopElement;
			vtx1LoopElement->SetNext(vtx2LoopElement, _mesh);
		}
	}
	else if(vtx2LoopElement != nullptr)
	{
		ASSERT(vtx1LoopElement == nullptr);
		if(vtx2LoopElement->GetNext() != nullptr)
		{
			ASSERT(vtx2LoopElement->GetPrev() == nullptr);
			if(vtx2LoopElement->GetPrev() != nullptr)
				return false;	// The loops might be build from non manifold data, reject this edge
			vtx1LoopElement = new LoopElement(idVtx1);
			_registeredLoopElements[idVtx1] = vtx1LoopElement;
			vtx2LoopElement->SetPrev(vtx1LoopElement, _mesh);
		}
		else
		{
			ASSERT(vtx2LoopElement->GetNext() == nullptr);
			if(vtx2LoopElement->GetNext() != nullptr)
				return false;	// The loops might be build from non manifold data, reject this edge
			vtx1LoopElement = new LoopElement(idVtx1);
			_registeredLoopElements[idVtx1] = vtx1LoopElement;
			vtx2LoopElement->SetNext(vtx1LoopElement, _mesh);
		}
	}
	else
	{
		vtx1LoopElement = new LoopElement(idVtx1);
		vtx2LoopElement = new LoopElement(idVtx2);
		vtx1LoopElement->SetNext(vtx2LoopElement, _mesh);
		_registeredLoopElements[idVtx1] = vtx1LoopElement;
		_registeredLoopElements[idVtx2] = vtx2LoopElement;
	}
	return true;
}

void LoopBuilder::BuildLoops(std::vector<Loop>& outLoops)
{
	ASSERT(outLoops.empty());
	// Consume _registeredLoopElements to create _loops
	while(_registeredLoopElements.empty() == false)
	{
		std::map<unsigned int, LoopElement*>::iterator itElement = _registeredLoopElements.begin();
		outLoops.push_back(Loop(itElement->second));
		while(itElement != _registeredLoopElements.end())
		{
			LoopElement* element = itElement->second;
			_registeredLoopElements.erase(itElement->first);
			if(element->GetNext() != nullptr)
				itElement = _registeredLoopElements.find(element->GetNext()->GetVtxId());
			else
				break;
		}
	}
}
