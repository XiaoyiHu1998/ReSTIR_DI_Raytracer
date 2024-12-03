#ifndef BVH_HPP
#define BVH_HPP

#include "AccelerationStructure.h"

class BVH : public AccelerationStructure
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

	struct Triangle
	{
		glm::vec3 vertex0, vertex1, vertex2;
		glm::vec3 centroid;

		Triangle(glm::vec3 vertex0, glm::vec3 vertex1, glm::vec3 vertex2) :
			vertex0{ vertex0 }, vertex1{ vertex1 }, vertex2{ vertex2 }
		{
			centroid = (vertex0 + vertex1 + vertex2) * 0.3333f;
		}
		
		bool Intersect(glm::vec3 origin, glm::vec3 direction, float& tnear) 
		{
			const glm::vec3 edge1 = vertex1 - vertex0;
			const glm::vec3 edge2 = vertex2 - vertex0;
			const glm::vec3 h = glm::cross(direction, edge2);
			const float a = glm::dot(edge1, h);
			if (a > -0.0001f && a < 0.0001f) return false; // ray parallel to triangle
			const float f = 1 / a;
			const glm::vec3 s = origin - vertex0;
			const float u = f * glm::dot(s, h);
			if (u < 0 || u > 1) return false;
			const glm::vec3 q = glm::cross(s, edge1);
			const float v = f * glm::dot(direction, q);
			if (v < 0 || u + v > 1) return false;
			const float t = f * glm::dot(edge2, q);
			if (t > 0.0001f) {
				tnear = tnear < t ? tnear : t;
				return true;
			}
			
			return false;
		}
	};
private:
	std::vector<BVHNode> m_BvhNodes;
	std::vector<Triangle> m_Triangles;
	std::vector<uint32_t> m_TriangleIndices;

	DebugMode m_DebugMode;
public:
	BVH(DebugMode debugMode = DebugMode::Off);
	virtual ~BVH() {}

	virtual void SetDebugMode(DebugMode debugMode);
	virtual bool Traverse(const glm::vec3& origin, const glm::vec3& direction, float& tnear) override;

	virtual void AddObject(const RenderObject& object) override;
	virtual void Build(bool useHeuristic) override;
private:
	void UpdateNodeBounds(uint32_t nodeID);
	void Subdivide(uint32_t nodeID, uint32_t& nodesUsed);
	bool TraverseNode(const glm::vec3& origin, const glm::vec3& direction, float& tnear, const uint32_t nodeIndex);
	bool IntersectAABB(glm::vec3 origin, glm::vec3 direction, float& tnear, glm::vec3 aabbMin, glm::vec3 aabbMax);
};


#endif //BVH_HPP