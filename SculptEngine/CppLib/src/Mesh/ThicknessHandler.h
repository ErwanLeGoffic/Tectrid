#ifndef _THICKNESS_HANDLER_H_
#define _THICKNESS_HANDLER_H_

#include <vector>
#include "Math\Math.h"

class Mesh;
class Loop;
class LoopElement;
class ThicknessHandler
{
public:
	ThicknessHandler(Mesh& mesh);

	enum FUSION_MODE
	{
		FUSION_MODE_PIERCE = 1,
		FUSION_MODE_MERGE
	};

#ifdef THICKNESS_HANDLER_WIP
	void AddVertexToTestList(unsigned int vtxIdx) { _verticesToTest.push_back(vtxIdx); }
	void AddVertexToMovedList(unsigned int vtxIdx) { _verticesMoved.push_back(vtxIdx); }
#else
	void AddVertexToTestList(unsigned int /*vtxIdx*/) { }
	void AddVertexToMovedList(unsigned int /*vtxIdx*/) { }
#endif // !THICKNESS_HANDLER_WIP
	void ReshapeRegardingThickness(FUSION_MODE fusionMode);

private:
	Loop CollectRingLoop(unsigned int vtxIdx);
	bool SmoothVertex(unsigned int vtxIdx);	// Return true if something was done

	Mesh& _mesh;
	std::vector<unsigned int>& _triangles;
	std::vector<Vector3>& _vertices;
	std::vector<Vector3> const& _normals;
	std::vector<unsigned int> _verticesToTest;
	std::vector<unsigned int> _verticesMoved;
	std::vector<float> _distances;
};

#endif // _THICKNESS_HANDLER_H_