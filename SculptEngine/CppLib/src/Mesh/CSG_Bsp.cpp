#include "CSG_Bsp.h"

static bool somethingWasRemoved = false;

Csg_Bsp Csg_Bsp::Subtract(Csg_Bsp const& input, bool inverThis, bool inverInput)
{
	std::unique_ptr<Csg_Bsp::BspNode> inputBspRoot(new Csg_Bsp::BspNode(input.GetPolygons()));
	if(inverInput)
		inputBspRoot->Invert();
	std::vector<Csg_Bsp::Polygon> result;
	for(Polygon polygon : _polygons)
	{
		if(inverThis)
			polygon.Flip();
		somethingWasRemoved = false;
		std::vector<Polygon> tmp;
		tmp.push_back(polygon);
		tmp = inputBspRoot->ClipPolygons(tmp);
		if(somethingWasRemoved)
			result.insert(result.end(), tmp.begin(), tmp.end());	// Concat
		else
			result.push_back(polygon);
	}
	return Csg_Bsp(result, true);
}

void Csg_Bsp::Plane::SplitPolygon(Polygon const& polygon, std::vector<Polygon>& coplanarFront, std::vector<Polygon>& coplanarBack, std::vector<Polygon>& front, std::vector<Polygon>& back)
{
	const unsigned int COPLANAR = 0;
	const unsigned int FRONT = 1;
	const unsigned int BACK = 2;
	const unsigned int SPANNING = 3;

	// Classify each point as well as the entire polygon into one of the above
	// four classes.
	unsigned int polygonType = 0;
	std::vector<unsigned int> types;
	for(Vertex const& vertex : polygon.GetVertices())
	{
		double dist = DistanceToVertex(vertex.GetPos());
		unsigned int type = (dist < -_EPSILON) ? BACK : (dist > _EPSILON) ? FRONT : COPLANAR;
		polygonType |= type;
		types.push_back(type);
	}

	// Put the polygon in the correct list, splitting it when necessary.
	switch(polygonType)
	{
	case COPLANAR:
		(_n.Dot(polygon.GetPlane().GetNormal()) > 0.0 ? coplanarFront : coplanarBack).push_back(polygon);
		break;
	case FRONT:
		front.push_back(polygon);
		break;
	case BACK:
		back.push_back(polygon);
		break;
	case SPANNING:
		std::vector<Vertex> f;
		std::vector<Vertex> b;
		for(unsigned int i = 0; i < (unsigned int) polygon.GetVertices().size(); ++i)
		{
			unsigned int j = (i + 1) % (unsigned int) polygon.GetVertices().size();
			unsigned int ti = types[i];
			unsigned int tj = types[j];
			Vertex vi = polygon.GetVertices()[i];
			Vertex vj = polygon.GetVertices()[j];
			if(ti != BACK)
				f.push_back(vi);
			if(ti != FRONT)
				b.push_back(vi);
			if((ti | tj) == SPANNING)
			{
				double t = (_d - _n.Dot(vi.GetPos())) / _n.Dot(vj.GetPos() - vi.GetPos());
				Vertex v = vi.Interpolate(vj, t);
				// Rounding error fixing
				/*double dist = DistanceToVertex(v.GetPos());
				v.GrabPos() -= _n * dist;
				dist = DistanceToVertex(v.GetPos());*/
				// Assign value
				f.push_back(v);
				b.push_back(v);
			}
		}
		if(f.size() >= 3) front.push_back(Csg_Bsp::Polygon(f, polygon.GetShared()));
		if(b.size() >= 3) back.push_back(Csg_Bsp::Polygon(b, polygon.GetShared()));
		break;
	}
}

void Csg_Bsp::BspNode::Invert()
{
	if(_plane != nullptr)
		_plane->Flip();
	if(_front)
		_front->Invert();
	if(_back)
		_back->Invert();
	_front.swap(_back);
}

std::vector<Csg_Bsp::Polygon> Csg_Bsp::BspNode::ClipPolygons(std::vector<Polygon>& inputPolygons)
{
	ASSERT(_plane != nullptr);
	if(_plane == nullptr)
		return inputPolygons;
	std::vector<Csg_Bsp::Polygon> front;
	std::vector<Csg_Bsp::Polygon> back;
	for(Polygon const& polygon : inputPolygons)
		_plane->SplitPolygon(polygon, front, back, front, back);
	if(_front != nullptr)
		front = _front->ClipPolygons(front);
	if(_back != nullptr)
		back = _back->ClipPolygons(back);
	else if(!back.empty())
	{
		back.clear();
		somethingWasRemoved = true;
	}
	front.insert(front.end(), back.begin(), back.end());	// Concat back into front
	return front;

	/*ASSERT(_plane != nullptr);
	if(_plane == nullptr)
		return inputPolygons;
	std::vector<Csg_Bsp::Polygon> result;
	std::vector<std::pair<Csg_Bsp::BspNode*, std::vector<Polygon>>> nodeToClipWith;
	nodeToClipWith.push_back(std::make_pair(this, std::move(inputPolygons)));
	while(!nodeToClipWith.empty())
	{
		Csg_Bsp::BspNode* curClipNode = nodeToClipWith.back().first;
		std::vector<Polygon> polygons = std::move(nodeToClipWith.back().second);
		nodeToClipWith.pop_back();
		std::vector<Csg_Bsp::Polygon> front;
		std::vector<Csg_Bsp::Polygon> back;
		for(Polygon const& polygon : polygons)
			curClipNode->_plane->SplitPolygon(polygon, front, back, front, back);
		
		if(curClipNode->_front == nullptr)
			result.insert(result.end(), front.begin(), front.end());	// Concat
		else if(!front.empty())
			nodeToClipWith.push_back(std::make_pair(curClipNode->_front.get(), front));

		if(curClipNode->_back == nullptr)
		{
			if(!back.empty())
			{
				//back.clear();
				somethingWasRemoved = true;
			}
		}
		else if(!back.empty())
			nodeToClipWith.push_back(std::make_pair(curClipNode->_back.get(), back));
	}
	return result;*/
}

void Csg_Bsp::BspNode::Build(std::vector<Polygon> inputPolygons)
{
	if(inputPolygons.empty())
		return;
	std::vector<std::pair<Csg_Bsp::BspNode*, std::vector<Polygon>>> nodeToBuild;
	nodeToBuild.push_back(std::make_pair(this, std::move(inputPolygons)));
	while(!nodeToBuild.empty())
	{
		Csg_Bsp::BspNode* curNode = nodeToBuild.back().first;
		std::vector<Polygon> polygons = std::move(nodeToBuild.back().second);
		nodeToBuild.pop_back();
		ASSERT(curNode->_plane == nullptr);
		curNode->_plane.reset(new Csg_Bsp::Plane(polygons[0].GetPlane()));
		std::vector<Csg_Bsp::Polygon> front;
		std::vector<Csg_Bsp::Polygon> back;
		std::vector<Csg_Bsp::Polygon> temp;
		for(unsigned int i = 1; i < polygons.size(); ++i)
			curNode->_plane->SplitPolygon(polygons[i], temp, temp, front, back);
		polygons.clear();
		polygons.shrink_to_fit();
		if(front.size() > 0)
		{
			ASSERT(curNode->_front == nullptr);
			curNode->_front.reset(new Csg_Bsp::BspNode());
			nodeToBuild.push_back(std::make_pair(curNode->_front.get(), front));
		}
		if(back.size() > 0)
		{
			ASSERT(curNode->_back == nullptr);
			curNode->_back.reset(new Csg_Bsp::BspNode());
			nodeToBuild.push_back(std::make_pair(curNode->_back.get(), back));
		}
	}
}
