#ifndef Octree_HPP
#define Octree_HPP

#include "AccelerationStructure.h"
#include "Triangle.h"

class Octree : public AccelerationStructure
{
private:
	struct OctreeNode
	{
		glm::vec3 aabbMin, aabbMax;
		OctreeNode* children[8];
		std::vector<uint32_t> triangleIndices;
		bool isLeaf;

		OctreeNode() :
			aabbMin{ glm::vec3(0) }, aabbMax{ glm::vec3(0) }, triangleIndices{ {} }, isLeaf{ true }
		{
		}
	};

	std::vector<Triangle> m_Triangles;
	OctreeNode m_RootNode;

	DebugMode m_DebugMode;

public:
	Octree(DebugMode debugMode = DebugMode::Off);
	virtual ~Octree() {}

	virtual void SetDebugMode(DebugMode debugMode);
	virtual bool Traverse(const glm::vec3& origin, const glm::vec3& direction, float& tnear) override;

	virtual void AddObject(const RenderObject& object) override;
	virtual void Build(bool useHeuristic) override;
private:

	void InitRootNode(OctreeNode& root);
	void Subdivide(OctreeNode& node);
	bool TriangleInAABB(Triangle triangle, glm::vec3 aabbMin, glm::vec3 aabbMax);
	bool TraverseNode(const glm::vec3& origin, const glm::vec3& direction, float& tnear, const OctreeNode node);
	bool IntersectAABB(glm::vec3 origin, glm::vec3 direction, float& tnear, glm::vec3 aabbMin, glm::vec3 aabbMax);
};


#endif //BVH_HPP#pragma once
