#include "Octree.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <iostream>

Octree::Octree(int maxDepth)
{
	m_Triangles = {};
	m_Triangles.reserve(10000 * 3);

	m_RootNode = OctreeNode();

	this->maxDepth = maxDepth;
}

bool Octree::Traverse(Ray& ray)
{
	//bool hit = TraverseNode(origin, direction, tnear, m_RootNode);
	//return hit;
	return false;
}

bool Octree::TraverseNode(Ray& ray, const OctreeNode node)
{
	bool hit = false;

	ray.boxIntersectionCount++;

	if (!IntersectAABB(ray.origin, ray.direction, ray.tnear, node.aabbMin, node.aabbMax))
		return false;

	ray.traversalSteps++;

	if (node.isLeaf)
		for (uint32_t i = 0; i < node.triangleIndices.size(); i++)
		{
			hit |= m_Triangles[node.triangleIndices[i]].Intersect(ray.origin, ray.direction, ray.tnear);
			ray.intersectionCount++;
		}
	else
		for (int i = 0; i < 8; i++)
			hit |= TraverseNode(ray, *node.children[i]);

	return hit;
}

bool Octree::IntersectAABB(glm::vec3 origin, glm::vec3 direction, float tnear, glm::vec3 aabbMin, glm::vec3 aabbMax)
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

void Octree::AddObject(const RenderObject& object)
{
	const Model& model = object.m_Model;
	int faceCount = model.nfaces();

	m_Triangles.reserve(m_Triangles.size() + faceCount * 3);

	for (int faceIndex = 0; faceIndex < faceCount; faceIndex++)
		m_Triangles.emplace_back(Triangle(model.point(model.vert(faceIndex, 0)), model.point(model.vert(faceIndex, 1)), model.point(model.vert(faceIndex, 2))));
}

void Octree::Build(bool useHeuristic)
{
	uint32_t rootNodeIndex = 0, nodesUsed = 1;

	// build root node
	InitRootNode(m_RootNode);

	std::cout << "Finished Building Root Node" << std::endl;

	// recursive subdivision for child nodes
	Subdivide(m_RootNode, 0);
	std::cout << "Finished Subdividing Root Node" << std::endl;

	std::cout << "Bounding box min: " << glm::to_string(m_RootNode.aabbMin) << std::endl;
	std::cout << "Bounding box max: " << glm::to_string(m_RootNode.aabbMax) << std::endl;

	int count = 0;
	for (int i = 0; i < m_Triangles.size(); i++)
	{
		bool triangleInAABB = TriangleInAABB(m_Triangles[i], m_RootNode.aabbMin, m_RootNode.aabbMax);
		if(triangleInAABB)
			count++;
	}
	std::cout << "Triangles in AABB: " << count << std::endl;
	std::cout << "Total triangles: " << m_Triangles.size() << std::endl;
}

void Octree::InitRootNode(OctreeNode& root)
{
	root.triangleIndices.resize(m_Triangles.size());

	for (int i = 0; i < m_Triangles.size(); i++)
		root.triangleIndices[i] = i;

	root.aabbMin = glm::vec3(1e30f);
	root.aabbMax = glm::vec3(-1e30f);

	for (int i = 0; i < root.triangleIndices.size(); i++)
	{
		Triangle& leafTriangle = m_Triangles[root.triangleIndices[i]];
		root.aabbMin = glm::min(root.aabbMin, leafTriangle.vertex0);
		root.aabbMin = glm::min(root.aabbMin, leafTriangle.vertex1);
		root.aabbMin = glm::min(root.aabbMin, leafTriangle.vertex2);
		root.aabbMax = glm::max(root.aabbMax, leafTriangle.vertex0);
		root.aabbMax = glm::max(root.aabbMax, leafTriangle.vertex1);
		root.aabbMax = glm::max(root.aabbMax, leafTriangle.vertex2);
	}

	const glm::vec3 center = (root.aabbMin + root.aabbMax) * 0.5f;
	const glm::vec3 diagonal = root.aabbMax - root.aabbMin;
	const float sideLength = std::max(diagonal.x, std::max(diagonal.y, diagonal.z));

	root.aabbMax = center + glm::vec3(sideLength * 0.5f);
	root.aabbMin = center - glm::vec3(sideLength * 0.5f);
}

void Octree::Subdivide(OctreeNode& node, int depth)
{
	// terminate recursion
	if (node.triangleIndices.size() <= 2 || depth >= maxDepth)
	{
		return;
	}

	if (depth == 1)
	{
		std::cout << "Bounding box min: " << glm::to_string(node.aabbMin) << std::endl;
		std::cout << "Bounding box max: " << glm::to_string(node.aabbMax) << std::endl;
		std::cout << "Triangle count: " << node.triangleIndices.size() << std::endl;
	}

	node.isLeaf = false;

	// determine center of node
	glm::vec3 center = (node.aabbMin + node.aabbMax) * 0.5f;

	// create children
	for (int i = 0; i < 8; i++)
	{
		node.children[i] = new OctreeNode();

		float xMin = i & 1 ? center.x : node.aabbMin.x;
		float xMax = i & 1 ? node.aabbMax.x : center.x;
		float yMin = i & 2 ? center.y : node.aabbMin.y;
		float yMax = i & 2 ? node.aabbMax.y : center.y;
		float zMin = i & 4 ? center.z : node.aabbMin.z;
		float zMax = i & 4 ? node.aabbMax.z : center.z;

		node.children[i]->aabbMin = glm::vec3(xMin, yMin, zMin);
		node.children[i]->aabbMax = glm::vec3(xMax, yMax, zMax);
	}

	for (int i = 0; i < node.triangleIndices.size(); i++)
		for (int j = 0; j < 8; j++)
		{
			OctreeNode& child = *node.children[j];
			if (TriangleInAABB(m_Triangles[node.triangleIndices[i]], child.aabbMin, child.aabbMax))
			{
				child.triangleIndices.push_back(node.triangleIndices[i]);
			}
		}

	for (int i = 0; i < 8; i++)
		Subdivide(*node.children[i], depth + 1);
}

// https://michael-schwarz.com/research/publ/files/vox-siga10.pdf
bool Octree::TriangleInAABB(Triangle triangle, glm::vec3 aabbMin, glm::vec3 aabbMax)
{
	// triangle bounding box overlap check
	glm::vec3 triangleMin = glm::min(triangle.vertex0, glm::min(triangle.vertex1, triangle.vertex2));
	glm::vec3 triangleMax = glm::max(triangle.vertex0, glm::max(triangle.vertex1, triangle.vertex2));

	if (triangleMin.x > aabbMax.x || triangleMax.x < aabbMin.x ||
		triangleMin.y > aabbMax.y || triangleMax.y < aabbMin.y ||
		triangleMin.z > aabbMax.z || triangleMax.z < aabbMin.z)
		return false;

	// triangle plane overlap check
	glm::vec3 edge0 = triangle.vertex1 - triangle.vertex0;
	glm::vec3 edge1 = triangle.vertex2 - triangle.vertex1;
	glm::vec3 edge2 = triangle.vertex0 - triangle.vertex2;
	glm::vec3 n = glm::cross(edge0, edge1);

	auto diagonal = aabbMax - aabbMin;

	glm::vec3 c = glm::vec3(
		n.x > 0 ? diagonal.x : 0,
		n.y > 0 ? diagonal.y : 0,
		n.z > 0 ? diagonal.z : 0);

	float d1 = glm::dot(n, c - triangle.vertex0);
	float d2 = glm::dot(n, diagonal - c - triangle.vertex0);

	if ((glm::dot(n, aabbMin) + d1) * (glm::dot(n, aabbMin) + d2) > 0.f)
		return false;

	// xy projection overlap check
	if (!TriangleProjectionInAABB(triangle, n, diagonal, aabbMin, 0, 1, 2))
		return false;

	// yz projection overlap check
	if(!TriangleProjectionInAABB(triangle, n, diagonal, aabbMin, 1, 2, 0))
		return false;

	// xz projection overlap check
	if (!TriangleProjectionInAABB(triangle, n, diagonal, aabbMin, 0, 2, 1))
		return false;

	return true;
}

bool Octree::TriangleProjectionInAABB(Triangle triangle, glm::vec3 n, glm::vec3 diagonal, glm::vec3 aabbMin, int firstAxis, int secondAxis, int nullspaceAxis)
{
	glm::vec3 edge0 = triangle.vertex1 - triangle.vertex0;
	glm::vec3 edge1 = triangle.vertex2 - triangle.vertex1;
	glm::vec3 edge2 = triangle.vertex0 - triangle.vertex2;

	float sign = n[nullspaceAxis] < 0 ? -1.f : 1.f;

	glm::vec2 edgeNormal0 = glm::vec2(-edge0[secondAxis], edge0[firstAxis]) * sign;
	glm::vec2 edgeNormal1 = glm::vec2(-edge1[secondAxis], edge1[firstAxis]) * sign;
	glm::vec2 edgeNormal2 = glm::vec2(-edge2[secondAxis], edge2[firstAxis]) * sign;

	float d0 = -glm::dot(edgeNormal0, glm::vec2(triangle.vertex0[firstAxis], triangle.vertex0[secondAxis])) + std::max(0.f, diagonal[firstAxis] * edgeNormal0[0]) + std::max(0.f, diagonal[secondAxis] * edgeNormal0[1]);
	float d1 = -glm::dot(edgeNormal1, glm::vec2(triangle.vertex1[firstAxis], triangle.vertex1[secondAxis])) + std::max(0.f, diagonal[firstAxis] * edgeNormal1[0]) + std::max(0.f, diagonal[secondAxis] * edgeNormal1[1]);
	float d2 = -glm::dot(edgeNormal2, glm::vec2(triangle.vertex2[firstAxis], triangle.vertex2[secondAxis])) + std::max(0.f, diagonal[firstAxis] * edgeNormal2[0]) + std::max(0.f, diagonal[secondAxis] * edgeNormal2[1]);

	bool condition0 = glm::dot(edgeNormal0, glm::vec2(aabbMin[firstAxis], aabbMin[secondAxis])) + d0 >= 0;
	bool condition1 = glm::dot(edgeNormal1, glm::vec2(aabbMin[firstAxis], aabbMin[secondAxis])) + d1 >= 0;
	bool condition2 = glm::dot(edgeNormal2, glm::vec2(aabbMin[firstAxis], aabbMin[secondAxis])) + d2 >= 0;

	return condition0 && condition1 && condition2;
}
