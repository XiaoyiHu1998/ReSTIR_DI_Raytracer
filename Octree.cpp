#include "Octree.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <iostream>

Octree::Octree(DebugMode debugMode) :
	m_DebugMode{ debugMode }
{
	m_Triangles = {};
	m_Triangles.reserve(10000 * 3);

	m_RootNode = OctreeNode();
}

void Octree::SetDebugMode(DebugMode debugMode)
{
	m_DebugMode = debugMode;
}

bool Octree::Traverse(const glm::vec3& origin, const glm::vec3& direction, float& tnear)
{
	bool hit = TraverseNode(origin, direction, tnear, m_RootNode);
	return hit;
}

bool Octree::TraverseNode(const glm::vec3& origin, const glm::vec3& direction, float& tnear, const OctreeNode node)
{
	bool hit = false;

	if (!IntersectAABB(origin, direction, tnear, node.aabbMin, node.aabbMax))
		return false;

	if (node.isLeaf)
		for (uint32_t i = 0; i < node.triangleIndices.size(); i++)
			hit |= m_Triangles[node.triangleIndices[i]].Intersect(origin, direction, tnear);
	else
		for (int i = 0; i < 8; i++)
			hit |= TraverseNode(origin, direction, tnear, node);

	return hit;
}

bool Octree::IntersectAABB(glm::vec3 origin, glm::vec3 direction, float& tnear, glm::vec3 aabbMin, glm::vec3 aabbMax)
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
	Subdivide(m_RootNode);
	std::cout << "Finished Subdividing Root Node" << std::endl;
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

void Octree::Subdivide(OctreeNode& node)
{
	// terminate recursion
	if (node.triangleIndices.size() <= 2)
		return;

	node.isLeaf = false;

	for (int i = 0; i < 8; i++)
		node.children[i] = new OctreeNode();

	// determine split axis and position
	glm::vec3 center = (node.aabbMin + node.aabbMax) * 0.5f;

	for (int i = 0; i < node.triangleIndices.size(); i++)
		for (int j = 0; j < 8; j++)
		{
			OctreeNode* child = node.children[j];
			if (TriangleInAABB(m_Triangles[node.triangleIndices[i]], child->aabbMin, child->aabbMax))
				child->triangleIndices.push_back(node.triangleIndices[i]);
		}

	for (int i = 0; i < 8; i++)
		Subdivide(*node.children[i]);
}

// https://omnigoat.github.io/2015/03/09/box-triangle-intersection/
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
	glm::vec3 n = glm::cross(edge0, -edge2);

	auto diagonal = aabbMax - aabbMin;

	glm::vec3 c = glm::vec3(
		n.x > 0 ? diagonal.x : 0,
		n.y > 0 ? diagonal.y : 0,
		n.z > 0 ? diagonal.z : 0);

	float d1 = glm::dot(n, c - triangle.vertex0);
	float d2 = glm::dot(n, diagonal - c - triangle.vertex0);

	if ((glm::dot(n, aabbMin) + d1) * (glm::dot(n, aabbMin) + d2) > 0.f)
		return false;

	// xy-plane projection overlap check
	float xym = (n.z < 0 ? -1.f : 1.f);

	glm::vec3 ne0xy = glm::vec3(-edge0.y, edge0.x, 0) * xym;
	glm::vec3 ne1xy = glm::vec3(-edge1.y, edge1.x, 0) * xym;
	glm::vec3 ne2xy = glm::vec3(-edge2.y, edge2.x, 0) * xym;

	glm::vec3 v0xy = glm::vec3(triangle.vertex0.x, triangle.vertex0.y, 0);
	glm::vec3 v1xy = glm::vec3(triangle.vertex1.x, triangle.vertex1.y, 0);
	glm::vec3 v2xy = glm::vec3(triangle.vertex2.x, triangle.vertex2.y, 0);

	float de0xy = -glm::dot(ne0xy, v0xy) + std::max(0.f, diagonal.x * ne0xy.x) + std::max(0.f, diagonal.y * ne0xy.y);
	float de1xy = -glm::dot(ne1xy, v1xy) + std::max(0.f, diagonal.x * ne1xy.x) + std::max(0.f, diagonal.y * ne1xy.y);
	float de2xy = -glm::dot(ne2xy, v2xy) + std::max(0.f, diagonal.x * ne2xy.x) + std::max(0.f, diagonal.y * ne2xy.y);

	glm::vec3 pxy = glm::vec3(aabbMin.x, aabbMax.y, 0);

	if ((glm::dot(ne0xy, pxy) + de0xy) < 0.f || (glm::dot(ne1xy, pxy) + de1xy) < 0.f || (glm::dot(ne2xy, pxy) + de2xy) < 0.f)
		return false;
}
