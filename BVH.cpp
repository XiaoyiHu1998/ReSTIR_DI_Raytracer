#include "BVH.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <iostream>

BVH::BVH(DebugMode debugMode):
	m_DebugMode{debugMode}
{
	m_Triangles = {};
	m_BvhNodes = {};

	m_BvhNodes.resize(30);
	m_Triangles.reserve(10000 * 3);
	m_TriangleIndices.reserve(10000 * 3);
}

void BVH::SetDebugMode(DebugMode debugMode)
{
	m_DebugMode = debugMode;
}

bool BVH::Traverse(const glm::vec3& origin, const glm::vec3& direction, float& tnear)
{
	bool hit = TraverseNode(origin, direction, tnear, 0);
	if (hit)
		std::cout << "hit: " << hit << std::endl;
	return hit;
}

bool BVH::TraverseNode(const glm::vec3& origin, const glm::vec3& direction, float& tnear, const uint32_t nodeIndex)
{
	BVHNode& node = m_BvhNodes[nodeIndex];
	bool hit = false;

	if (!IntersectAABB(origin, direction, tnear, node.aabbMin, node.aabbMax)) 
		return false;

	if (node.isLeaf())
	{
		std::cout << "primCount: " << node.triangleCount << std::endl;
		for (uint32_t i = 0; i < node.triangleCount; i++)
		{
			hit |= m_Triangles[m_TriangleIndices[node.leftChildOrFirstIndex + i]].Intersect(origin, direction, tnear);
		}
	}
	else
	{
		hit |= TraverseNode(origin, direction, tnear, node.leftChildOrFirstIndex);
		hit |= TraverseNode(origin, direction, tnear, node.leftChildOrFirstIndex + 1);
	}

	return hit;
}

bool BVH::IntersectAABB(glm::vec3 origin, glm::vec3 direction, float& tnear, glm::vec3 aabbMin, glm::vec3 aabbMax)
{
	float tXMin = (aabbMin.x - origin.x) / direction.x;
	float tXMax = (aabbMax.x - origin.x) / direction.x;
	float tmin = std::min(tXMin, tXMax);
	float tmax = std::max(tXMin, tXMax);

	float tYMin = (aabbMin.y - origin.y) / direction.y;
	float tYMax = (aabbMax.y - origin.y) / direction.y;
	tmin = std::max(tmin, std::min(tYMin, tYMax));
	tmax = std::min(tmin, std::max(tYMin, tYMax));

	float tZMin = (aabbMin.z - origin.z) / direction.z;
	float tZMax = (aabbMax.z - origin.z) / direction.z;
	tmin = std::max(tmin, std::min(tZMin, tZMax));
	tmax = std::min(tmin, std::max(tZMin, tZMax));

	return tmax >= tmin && tmin < tnear && tmax > 0;
}

void BVH::AddObject(const RenderObject& object)
{
	const Model& model = object.m_Model;
	int faceCount = model.nfaces();

	m_Triangles.reserve(m_Triangles.size() + faceCount * 3);

	for (int faceIndex = 0; faceIndex < faceCount; faceIndex++) {
		m_Triangles.emplace_back(Triangle(model.point(model.vert(faceIndex, 0)), model.point(model.vert(faceIndex, 1)), model.point(model.vert(faceIndex, 2))));
	}
}

void BVH::Build(bool useHeuristic)
{
	m_TriangleIndices.resize(m_Triangles.size());
	for (int i = 0; i < m_Triangles.size(); i++) {
		m_TriangleIndices[i] = i;
	}
	std::cout << "Finished Setting Indices" << std::endl;

	uint32_t rootNodeIndex = 0, nodesUsed = 1;
	//build root node
	BVHNode& root = m_BvhNodes[rootNodeIndex];
	root.leftChildOrFirstIndex = 0;
	root.triangleCount = m_Triangles.size();
	UpdateNodeBounds(rootNodeIndex);
	std::cout << "Finished Building Root Node" << std::endl;

	//recursive subdivision for child nodes
	Subdivide(rootNodeIndex, nodesUsed);
	std::cout << "Finished Subdividing Root Node" << std::endl;
}

void BVH::UpdateNodeBounds(uint32_t nodeID)
{
	BVHNode& node = m_BvhNodes[nodeID];
	node.aabbMin = glm::vec3(1e30f);
	node.aabbMax = glm::vec3(-1e30f);

	uint32_t firstChild = node.leftChildOrFirstIndex;
	for (int i = 0; i < node.triangleCount; i++)
	{
		uint32_t& leafTriangleIndex = m_TriangleIndices[firstChild + i];
		Triangle& leafTriangle = m_Triangles[leafTriangleIndex];
		node.aabbMin = glm::min(node.aabbMin, leafTriangle.vertex0);
		node.aabbMin = glm::min(node.aabbMin, leafTriangle.vertex1);
		node.aabbMin = glm::min(node.aabbMin, leafTriangle.vertex2);
		node.aabbMax = glm::max(node.aabbMax, leafTriangle.vertex0);
		node.aabbMax = glm::max(node.aabbMax, leafTriangle.vertex1);
		node.aabbMax = glm::max(node.aabbMax, leafTriangle.vertex2);
	}
}

void BVH::Subdivide(uint32_t nodeID, uint32_t& nodesUsed)
{
	BVHNode& node = m_BvhNodes[nodeID];

	//terminate recursion
	if (node.triangleCount <= 2) 
		return;

	//determine split axis and position
	glm::vec3 extent = node.aabbMax - node.aabbMin;
	int axis = 0;
	if (extent.y > extent.x) axis = 1;
	if (extent.z > extent[axis]) axis = 2;
	float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;

	//in-place partition
	int i = node.leftChildOrFirstIndex;
	int j = i + node.triangleCount - 1;
	while (i <= j)
	{
		if (m_Triangles[m_TriangleIndices[i]].centroid[axis] < splitPos) {
			i++;
		}
		else {
			uint32_t tempTriangleIndex = m_TriangleIndices[i];
			m_TriangleIndices[i] = m_TriangleIndices[j];
			m_TriangleIndices[j] = tempTriangleIndex;
			j--;
		}
	}

	//abort split if one side is empty
	int leftCount = i - node.leftChildOrFirstIndex;
	if (leftCount == 0 || leftCount == node.triangleCount) 
		return;

	// create child nodes
	int leftChildIndex = nodesUsed++;
	int rightChildIndex = nodesUsed++;
	if (rightChildIndex > m_BvhNodes.size() - 1)
	{
		for (int index = 0; index < rightChildIndex - (m_BvhNodes.size() - 1); index++) {
			m_BvhNodes.emplace_back(BVHNode());
		}
	}

	m_BvhNodes[leftChildIndex].leftChildOrFirstIndex = node.leftChildOrFirstIndex;
	m_BvhNodes[leftChildIndex].triangleCount = leftCount;
	m_BvhNodes[rightChildIndex].leftChildOrFirstIndex = i;
	m_BvhNodes[rightChildIndex].triangleCount = node.triangleCount - leftCount;
	node.leftChildOrFirstIndex = leftChildIndex;
	node.triangleCount = 0;
	UpdateNodeBounds(leftChildIndex);
	UpdateNodeBounds(rightChildIndex);

	//recurse
	Subdivide(leftChildIndex, nodesUsed);
	Subdivide(rightChildIndex, nodesUsed);
}
