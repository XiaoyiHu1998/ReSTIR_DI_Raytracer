#ifndef BVH_HPP
#define BVH_HPP

#include "AccelerationStructure.h"
#include "Triangle.h"

class BVH_BLAS : public AccelerationStructure
{
private:
	struct BVHNode
	{
		glm::vec3 aabbMin, aabbMax;
		uint32_t leftChildOrFirstIndex, triangleCount;

		BVHNode():
			aabbMin{ glm::vec3(0) }, aabbMax{ glm::vec3(0) }, leftChildOrFirstIndex{0}, triangleCount{0}
		{}
		const bool isLeaf() { return triangleCount > 0; }
	};

private:
	std::vector<BVHNode> m_BvhNodes;
	std::vector<Triangle> m_Triangles;
	std::vector<uint32_t> m_TriangleIndices;

	DebugMode m_DebugMode;
public:
	BVH_BLAS(DebugMode debugMode = DebugMode::Off);
	virtual ~BVH_BLAS() {};

	virtual void SetDebugMode(DebugMode debugMode);
	virtual bool Traverse(const glm::vec3& origin, const glm::vec3& direction, float& tnear) override;

	virtual void AddObject(const RenderObject& object) override;
	virtual void Build(bool useHeuristic) override;
	virtual int ObjectCount() override { return 1; }
private:
	void UpdateNodeBounds(uint32_t nodeID);
	void Subdivide(uint32_t nodeID, uint32_t& nodesUsed);
	bool TraverseNode(const glm::vec3& origin, const glm::vec3& direction, float& tnear, const uint32_t nodeIndex);
	bool IntersectAABB(glm::vec3 origin, glm::vec3 direction, float& tnear, glm::vec3 aabbMin, glm::vec3 aabbMax);
};


class BVH : public AccelerationStructure
{
private:
	std::vector<std::pair<BVH_BLAS, glm::mat4>> m_BVHObjects;
	DebugMode m_DebugMode;

public:
	BVH(DebugMode debubMode);
	virtual ~BVH() {};

	virtual void SetDebugMode(DebugMode debugMode);
	virtual bool Traverse(const glm::vec3& origin, const glm::vec3& direction, float& tnear) override;

	virtual void AddObject(const RenderObject& object) override;
	virtual void Build(bool useHeuristic) override;
	virtual int ObjectCount() override { return m_BVHObjects.size(); }
};


#endif //BVH_HPP