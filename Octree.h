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
			aabbMin{ glm::vec3(0) }, aabbMax{ glm::vec3(0) }, triangleIndices{ std::vector<uint32_t>() }, isLeaf{ true }
		{
		}
	};

	std::vector<Triangle> m_Triangles;
	OctreeNode m_RootNode;

public:
	Octree(int maxDepth = 32);
	virtual ~Octree() {}

	//virtual bool Traverse(const glm::vec3& origin, const glm::vec3& direction, float& tnear) override;
	virtual bool Traverse(Ray& ray) override;
	virtual glm::vec3 RandomTrianglePoint() const override;

	virtual void AddObject(const RenderObject& object) override;
	virtual void Build(bool useHeuristic) override;
	virtual int ObjectCount() override { return 1; }

	int maxDepth;
private:
	void InitRootNode(OctreeNode& root);
	void Subdivide(OctreeNode& node, int depth);
	bool TriangleInAABB(Triangle triangle, glm::vec3 aabbMin, glm::vec3 aabbMax);
	bool TriangleProjectionInAABB(Triangle triangle, glm::vec3 n, glm::vec3 diagonal, glm::vec3 aabbMin, int firstAxis, int secondAxis, int nullspaceAxis);
	bool TraverseNode(Ray& ray, const OctreeNode node);
	bool IntersectAABB(glm::vec3 origin, glm::vec3 direction, float tnear, glm::vec3 aabbMin, glm::vec3 aabbMax);
};


#endif //BVH_HPP#pragma once
