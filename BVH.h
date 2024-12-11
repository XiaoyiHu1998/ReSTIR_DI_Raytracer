#ifndef BVH_HPP
#define BVH_HPP

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include "AccelerationStructure.h"
#include "Triangle.h"
#include "Material.h"
#include "Utils.h"

class BVH_BLAS
{
private:
	struct BVHNode
	{
		glm::vec3 aabbMin, aabbMax;
		uint32_t leftChildOrFirstIndex, triangleCount;

		BVHNode():
			aabbMin{ glm::vec3(0) }, aabbMax{ glm::vec3(0) }, leftChildOrFirstIndex{0}, triangleCount{0}
		{}
		const bool isLeaf() const { return triangleCount > 0; }
	};

	struct AABB
	{
		glm::vec3 boundsMin = glm::vec3(std::numeric_limits<float>().min());
		glm::vec3 boundsMax = glm::vec3(std::numeric_limits<float>().max());

		void Grow(glm::vec3 vertex)
		{
			boundsMin = glm::min(boundsMin, vertex);
			boundsMax = glm::max(boundsMax, vertex);
		}

		float Area()
		{
			glm::vec3 extent = boundsMax - boundsMin;
			return extent.x * extent.y + extent.y * extent.z + extent.z * extent.x;
		}
	};

private:
	std::vector<BVHNode> m_BvhNodes;
	std::vector<Triangle> m_Triangles;
	std::vector<uint32_t> m_TriangleIndices;
	float m_Area;

	AccelerationStructure::DebugMode m_DebugMode;
public:
	BVH_BLAS(AccelerationStructure::DebugMode debugMode = AccelerationStructure::DebugMode::Off);
	~BVH_BLAS() {};

	bool Traverse(Ray& ray) const;
	TrianglePoint RandomTrianglePoint(uint32_t& seed) const;

	void AddObject(const RenderObject& object, const glm::mat4& transform);
	void Build(bool useHeuristic);

	void SetDebugMode(AccelerationStructure::DebugMode debugMode);
	int ObjectCount() { return 1; }

	float Area() const { return m_Area; }
private:
	void UpdateNodeBounds(uint32_t nodeID);
	void Subdivide(uint32_t nodeID, uint32_t& nodesUsed, bool useHeuristic);
	bool TraverseNode(Ray& ray, const uint32_t nodeIndex) const;
	bool IntersectAABB(glm::vec3 origin, glm::vec3 direction, float& tnear, glm::vec3 aabbMin, glm::vec3 aabbMax) const;
	float EvaluateSAH(BVHNode& node, int axis, float position);
};

struct BVH_Object
{
	BVH_BLAS bvh;
	Material material;
	glm::mat4 inverseTransform;

	BVH_Object(BVH_BLAS bvh, Material material, glm::mat4 inverseTransform):
		bvh{bvh}, material{material}, inverseTransform{inverseTransform}
	{}
};


class BVH : public AccelerationStructure
{
private:
	std::vector<BVH_Object> m_BVHObjects;
	DebugMode m_DebugMode;

public:
	BVH(DebugMode debubMode);
	virtual ~BVH() {};

	virtual bool Traverse(Ray& ray) override;
	virtual RandomLightPoint RandomTrianglePoint(uint32_t& seed) const override;

	virtual void AddObject(const RenderObject& object) override;
	virtual void Build(bool useHeuristic) override;

	virtual void SetDebugMode(DebugMode debugMode);
	virtual int ObjectCount() override { return m_BVHObjects.size(); }
};


#endif //BVH_HPP