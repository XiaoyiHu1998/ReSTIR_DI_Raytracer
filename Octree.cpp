#include "Octree.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <iostream>
#include <random>

Octree::Octree(int maxDepth)
{
	m_Triangles = {};
	m_Triangles.reserve(10000 * 3);

	m_RootNode = OctreeNode();

	this->maxDepth = maxDepth;
}

bool Octree::Traverse(Ray& ray)
{
	bool hit = TraverseNode(ray, m_RootNode);
	return hit;
}

bool Octree::TraverseNode(Ray& ray, const OctreeNode node)
{
	bool hit = false;

	ray.boxIntersectionCount++;

	if (!IntersectAABB(ray.origin, ray.direction, ray.tnear, node.aabbMin, node.aabbMax))
		return false;

	ray.traversalSteps++;

	if (node.isLeaf)
	{
		float closestDistance = ray.tnear;
		glm::vec3 normal = glm::vec3(0);

		for (uint32_t i = 0; i < node.triangleIndices.size(); i++)
		{
			const Triangle& currentTriangle = m_Triangles[node.triangleIndices[i]];
			float triangleDistance = std::numeric_limits<float>().max();
			bool triangleHit = currentTriangle.Intersect(ray.origin, ray.direction, triangleDistance);
			ray.intersectionCount++;

			if (triangleHit && triangleDistance < closestDistance)
			{
				closestDistance = triangleDistance;
				normal = currentTriangle.normal;
				hit = true;
			}
		}

		if (hit)
		{
			ray.tnear = closestDistance;
			ray.normal = normal;
		}

		return hit;
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			hit |= TraverseNode(ray, *node.children[i]);
		}
		//int closestChildIndex = GetClosestChildIndex((node.aabbMin + node.aabbMax) * 0.5f, ray.origin);

		//float t0, t1, t2;
		//bool planeHit0 = RayPlaneIntersection(ray, 0, (node.aabbMin + node.aabbMax) * 0.5f, (node.aabbMax.x - node.aabbMin.x) * 0.5f, t0);
		//bool planeHit1 = RayPlaneIntersection(ray, 1, (node.aabbMin + node.aabbMax) * 0.5f, (node.aabbMax.y - node.aabbMin.y) * 0.5f, t1);
		//bool planeHit2 = RayPlaneIntersection(ray, 2, (node.aabbMin + node.aabbMax) * 0.5f, (node.aabbMax.z - node.aabbMin.z) * 0.5f, t2);

		//float plane_hit[3] = { t0, t1, t2 };

		//for (int i = 0; i < 4; i++)
		//{
		//	hit |= TraverseNode(ray, *node.children[closestChildIndex]);
		//	if (hit)
		//		break;

		//	int planeIndex = -1;
		//	float closestPlaneHit = std::numeric_limits<float>::max();
		//	for (int j = 0; j < 3; j++)
		//	{
		//		if (plane_hit[j] < closestPlaneHit)
		//		{
		//			closestPlaneHit = plane_hit[j];
		//			planeIndex = j;
		//		}
		//	}
		//	if (planeIndex == -1 || closestPlaneHit > ray.tnear)
		//		break;

		//	closestChildIndex ^= 0x1 << planeIndex;
		//	plane_hit[planeIndex] = std::numeric_limits<float>::max();
		//}
	}

	return hit;
}

glm::vec3 Octree::RandomTrianglePoint() const
{
	std::srand(std::time(nullptr));

	int randomTriangleIndex = std::rand() % m_Triangles.size();
	const Triangle& randomTriangle = m_Triangles[randomTriangleIndex];

	float weightVertex0 = ((float(std::rand()) / RAND_MAX)) + 1;
	float maxWeightVertex1 = 1.0f - weightVertex0;
	float weightVertex1 = (((float(std::rand()) / RAND_MAX)) + 1) * maxWeightVertex1;
	float weightVertex2 = maxWeightVertex1 - weightVertex1;

	return weightVertex0 * randomTriangle.vertex0 + weightVertex1 * randomTriangle.vertex1 + weightVertex2 * randomTriangle.vertex2;
}

bool Octree::RayPlaneIntersection(const Ray ray, const int axis, const glm::vec3 planeCenter, const float span, float& t)
{
	if (ray.direction[axis] == 0)
		return false;

	t = (planeCenter[axis] - ray.origin[axis]) / ray.direction[axis];

	if (t < 0)
		return false;

	glm::vec3 intersection = ray.origin + ray.direction * t;

	if (intersection[(axis + 1) % 3] < planeCenter[(axis + 1) % 3] - span || intersection[(axis + 1) % 3] > planeCenter[(axis + 1) % 3] + span ||
		intersection[(axis + 2) % 3] < planeCenter[(axis + 2) % 3] - span || intersection[(axis + 2) % 3] > planeCenter[(axis + 2) % 3] + span)
		return false;

	return true;
}

int Octree::GetClosestChildIndex(const glm::vec3 nodeCenter, const glm::vec3 point)
{
	glm::vec3 direction = point - nodeCenter;

	int x_test = direction.x > 0 ? 1 : 0;
	int y_test = direction.y > 0 ? 1 : 0;
	int z_test = direction.z > 0 ? 1 : 0;

	return x_test | (y_test << 1) | (z_test << 2);
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
	const glm::mat4 transform = object.m_Transform.GetTransformMatrix();
	int faceCount = model.nfaces();

	m_Triangles.reserve(m_Triangles.size() + faceCount * 3);

	for (int faceIndex = 0; faceIndex < faceCount; faceIndex++)
	{
		glm::vec3 vertex0 = glm::vec3(glm::vec4(model.point(model.vert(faceIndex, 0)), 1) * transform);
		glm::vec3 vertex1 = glm::vec3(glm::vec4(model.point(model.vert(faceIndex, 1)), 1) * transform);
		glm::vec3 vertex2 = glm::vec3(glm::vec4(model.point(model.vert(faceIndex, 2)), 1) * transform);
		m_Triangles.emplace_back(Triangle(vertex0, vertex1, vertex2));
	}
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
		if (triangleInAABB)
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

	// clean up parent node
	node.triangleIndices.clear();

	for (int i = 0; i < 8; i++)
	{
		Subdivide(*node.children[i], depth + 1);
	}
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
	if (!TriangleProjectionInAABB(triangle, n, diagonal, aabbMin, 1, 2, 0))
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
