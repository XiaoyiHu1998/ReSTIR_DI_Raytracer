#include "BVH.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"

BVH::BVH()
{
	triangles = {};
	bvhNodes = {};

	triangles.reserve(10000 * 3);
	bvhNodes.reserve(30);
}

bool BVH::RayHit(glm::vec3 origin, glm::vec3 direction, float& tnear)
{
	return false;
}

void BVH::AddObject(const RenderObject& object)
{
	const Model& model = object.m_Model;
	int faceCount = model.nfaces();

	triangles.reserve(triangles.size() + faceCount * 3);

	for (int faceIndex = 0; faceIndex < faceCount; faceIndex++) {
		triangles.emplace_back(Triangle(model.point(model.vert(faceIndex, 0)), model.point(model.vert(faceIndex, 1)), model.point(model.vert(faceIndex, 2))));
	}
}

void BVH::Build(bool useHeuristic)
{
	uint32_t rootNodeID = 0, nodesUsed = 1;
	for (int i = 0; i < triangles.size(); i++) {
		BVHNode& root = bvhNodes[rootNodeID];
		root.leftFirst = 0;
		root.triangleCount = triangles.size();
		UpdateNodeBounds(rootNodeID);
		//Subdivide(rootNodeID);
	}
}

void BVH::UpdateNodeBounds(uint32_t nodeID)
{
	BVHNode& node = bvhNodes[nodeID];
	node.aabbMin = glm::vec3(1e30f);
	node.aabbMax = glm::vec3(-1e30f);

	for (int i = 0; i < node.triangleCount; i++)
	{
		uint32_t first = node.leftFirst;
		Triangle& leafTri = triangles[first + i];
		node.aabbMin = glm::min(node.aabbMin, leafTri.vertex0);
		node.aabbMin = glm::min(node.aabbMin, leafTri.vertex1);
		node.aabbMin = glm::min(node.aabbMin, leafTri.vertex2);
		node.aabbMax = glm::max(node.aabbMax, leafTri.vertex0);
		node.aabbMax = glm::max(node.aabbMax, leafTri.vertex1);
		node.aabbMax = glm::max(node.aabbMax, leafTri.vertex2);
	}
}

void BVH::Subdivide(uint32_t nodeID)
{
	BVHNode& node = bvhNodes[nodeID];

	glm::vec3 extent = node.aabbMax - node.aabbMin;
	int axis = 0;
	if (extent.y > extent.x) axis = 1;
	if (extent.z > extent[axis]) axis = 2;
	float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;

	int i = node.leftFirst;
	int j = i + node.triangleCount - 1;
	while (i <= j)
	{
		if (triangles[i].centroid[axis] < splitPos) {
			i++;
		}
		else {
			swap(tri[i], tri[j--]);
		}
	}
}
