#ifndef KdTree_HPP
#define KdTree_HPP

#include "AccelerationStructure.h"
#include "Triangle.h"

class KdTree : public AccelerationStructure
{
private:
	struct KdTreeNode
	{
		glm::vec3 aabbMin, aabbMax;
		KdTreeNode* children[2];
		std::vector<uint32_t> triangleIndices;
		bool isLeaf;

		KdTreeNode() :
			aabbMin{ glm::vec3(0) }, aabbMax{ glm::vec3(0) }, triangleIndices{ {} }, isLeaf{ true }
		{
		}
	};

	std::vector<Triangle> m_Triangles;
	KdTreeNode m_RootNode;

public:
	KdTree(int maxDepth = 32);
	virtual ~KdTree() {}

	virtual bool Traverse(Ray& ray) override;

	virtual void AddObject(const RenderObject& object) override;
	virtual void Build(bool useHeuristic) override;
	virtual int ObjectCount() override { return 1; }

	int maxDepth;
private:
	void InitRootNode(KdTreeNode& root);
	void Subdivide(KdTreeNode& node, int depth);
	bool TriangleInAABB(Triangle triangle, glm::vec3 aabbMin, glm::vec3 aabbMax);
	bool TriangleProjectionInAABB(Triangle triangle, glm::vec3 n, glm::vec3 diagonal, glm::vec3 aabbMin, int firstAxis, int secondAxis, int nullspaceAxis);
	bool TraverseNode(Ray& ray, const KdTreeNode node);
	bool IntersectAABB(glm::vec3 origin, glm::vec3 direction, float tnear, glm::vec3 aabbMin, glm::vec3 aabbMax);
};


#endif //BVH_HPP#pragma once
