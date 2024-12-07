#include "BVH.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <iostream>

BVH::BVH(DebugMode debubMode) :
	m_DebugMode{ debubMode }
{
	m_BVHObjects.reserve(10);
}

void BVH::SetDebugMode(DebugMode debugMode)
{
	m_DebugMode = debugMode;

	for (int i = 0; i < m_BVHObjects.size(); i++)
	{
		m_BVHObjects[i].first.SetDebugMode(debugMode);
	}
}

bool BVH::Traverse(const glm::vec3& origin, const glm::vec3& direction, float& tnear)
{
	bool hit = false;

	for (int i = 0; i < m_BVHObjects.size(); i++)
	{
		glm::mat4& inverseTransform = m_BVHObjects[i].second;
		glm::vec3 originTransformed = glm::vec3(inverseTransform * glm::vec4(origin, 1));
		glm::vec3 directionTransformed = glm::vec3(inverseTransform * glm::vec4(direction, 0));

		hit |= m_BVHObjects[i].first.Traverse(originTransformed, directionTransformed, tnear);
	}

	return hit;
}

void BVH::AddObject(const RenderObject& object)
{
	BVH_BLAS newBVH = BVH_BLAS(m_DebugMode);
	newBVH.AddObject(object);

	glm::mat4 inverseTransformMatrix = glm::inverse(object.m_Transform.GetTransformMatrix());

	m_BVHObjects.emplace_back(std::pair<BVH_BLAS, glm::mat4>(newBVH, inverseTransformMatrix));
}

void BVH::Build(bool useHeuristic)
{
	for (int i = 0; i < m_BVHObjects.size(); i++)
	{
		m_BVHObjects[i].first.Build(useHeuristic);
	}
}

BVH_BLAS::BVH_BLAS(DebugMode debugMode):
	m_DebugMode{debugMode}
{
	m_Triangles = {};
	m_BvhNodes = {};

	m_BvhNodes.resize(1000);
	m_Triangles.reserve(10000 * 3);
	m_TriangleIndices.reserve(10000 * 3);
}

void BVH_BLAS::SetDebugMode(DebugMode debugMode)
{
	m_DebugMode = debugMode;
}

bool BVH_BLAS::Traverse(const glm::vec3& origin, const glm::vec3& direction, float& tnear)
{
	bool hit = TraverseNode(origin, direction, tnear, 0);
	return hit;
}

bool BVH_BLAS::TraverseNode(const glm::vec3& origin, const glm::vec3& direction, float& tnear, const uint32_t nodeIndex)
{
	BVHNode& node = m_BvhNodes[nodeIndex];
	bool hit = false;

	if (!IntersectAABB(origin, direction, tnear, node.aabbMin, node.aabbMax)) 
		return false;

	if (node.isLeaf())
	{
		//std::cout << "primCount: " << node.triangleCount << std::endl;
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

bool BVH_BLAS::IntersectAABB(glm::vec3 origin, glm::vec3 direction, float& tnear, glm::vec3 aabbMin, glm::vec3 aabbMax)
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

void BVH_BLAS::AddObject(const RenderObject& object)
{
	const Model& model = object.m_Model;
	int faceCount = model.nfaces();

	m_Triangles.reserve(m_Triangles.size() + faceCount * 3);

	for (int faceIndex = 0; faceIndex < faceCount; faceIndex++) {
		m_Triangles.emplace_back(Triangle(model.point(model.vert(faceIndex, 0)), model.point(model.vert(faceIndex, 1)), model.point(model.vert(faceIndex, 2))));
	}
}

void BVH_BLAS::Build(bool useHeuristic)
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

void BVH_BLAS::UpdateNodeBounds(uint32_t nodeID)
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

void BVH_BLAS::Subdivide(uint32_t nodeID, uint32_t& nodesUsed)
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
			std::swap(m_TriangleIndices[i], m_TriangleIndices[j--]);
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
		m_BvhNodes.resize(rightChildIndex);
	}

	if (node.leftChildOrFirstIndex > 507 || i > 507)
	{
		int abcd = 1;
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
