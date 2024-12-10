#include <iostream>
#include <random>

#include "BVH.h"

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
		m_BVHObjects[i].bvh.SetDebugMode(debugMode);
	}
}

bool BVH::Traverse(Ray& ray)
{
	bool hit = false;

	for (const BVH_Object& bvhObject : m_BVHObjects)
	{
		const glm::mat4& inverseTransform = bvhObject.inverseTransform;
		const glm::mat4& objectTransform = glm::inverse(bvhObject.inverseTransform);

		Ray objectRay;
		objectRay.origin = glm::vec3(inverseTransform * glm::vec4(ray.origin, 1));
		objectRay.direction = glm::vec3(inverseTransform * glm::vec4(ray.direction, 0));

		if (bvhObject.bvh.Traverse(objectRay))
		{
			hit = true;

			ray.tnear = objectRay.tnear;
			ray.normal = glm::normalize(glm::vec3(objectTransform * glm::vec4(objectRay.normal, 0)));
			ray.material = bvhObject.material;
			ray.hitLocation = ray.origin + ray.direction * ray.tnear;
		}
	}

	return hit;
}

RandomLightPoint BVH::RandomTrianglePoint() const
{
	std::srand(std::time(nullptr));
	int randomBVHObjectIndex = std::rand() % m_BVHObjects.size();
	const BVH_Object& randomBVHObject = m_BVHObjects[randomBVHObjectIndex];
	TrianglePoint& trianglePoint = randomBVHObject.bvh.RandomTrianglePoint();
	return RandomLightPoint(trianglePoint.point, randomBVHObject.material.color, randomBVHObject.bvh.Area(), trianglePoint.normal, randomBVHObject.inverseTransform);
}

void BVH::AddObject(const RenderObject& object)
{
	BVH_BLAS newBVH = BVH_BLAS(m_DebugMode);
	newBVH.AddObject(object, object.m_Transform.GetTransformMatrix());

	glm::mat4 inverseTransformMatrix = glm::inverse(object.m_Transform.GetTransformMatrix());

	m_BVHObjects.emplace_back(BVH_Object(newBVH, object.m_Material, inverseTransformMatrix));
}

void BVH::Build(bool useHeuristic)
{
	for (int i = 0; i < m_BVHObjects.size(); i++)
	{
		m_BVHObjects[i].bvh.Build(useHeuristic);
	}
}

BVH_BLAS::BVH_BLAS(AccelerationStructure::DebugMode debugMode):
	m_DebugMode{debugMode}
{
	m_Triangles = {};
	m_BvhNodes = {};

	m_BvhNodes.resize(1000);
	m_Triangles.reserve(10000 * 3);
	m_TriangleIndices.reserve(10000 * 3);
}

void BVH_BLAS::SetDebugMode(AccelerationStructure::DebugMode debugMode)
{
	m_DebugMode = debugMode;
}

bool BVH_BLAS::Traverse(Ray& ray) const
{
	return TraverseNode(ray, 0);
}

TrianglePoint BVH_BLAS::RandomTrianglePoint() const
{
	std::srand(std::time(nullptr));
	
	int randomTriangleIndex = std::rand() % m_Triangles.size();
	const Triangle& randomTriangle = m_Triangles[randomTriangleIndex];
	
	float weightVertex0 = ((float(std::rand()) / RAND_MAX)) + 1;
	float maxWeightVertex1 = 1.0f - weightVertex0;
	float weightVertex1 = (((float(std::rand()) / RAND_MAX)) + 1) * maxWeightVertex1;
	float weightVertex2 = maxWeightVertex1 - weightVertex1;
	
	glm::vec3 trianglePoint = weightVertex0 * randomTriangle.vertex0 + weightVertex1 * randomTriangle.vertex1 + weightVertex2 * randomTriangle.vertex2;
	glm::vec3 normal = randomTriangle.normal;
	return TrianglePoint(trianglePoint, normal);
}

bool BVH_BLAS::TraverseNode(Ray& ray, const uint32_t nodeIndex) const
{
	const BVHNode& node = m_BvhNodes[nodeIndex];
	bool hit = false;

	if (!IntersectAABB(ray.origin, ray.direction, ray.tnear, node.aabbMin, node.aabbMax)) 
		return false;

	if (node.isLeaf())
	{
		float closestDistance = ray.tnear;
		glm::vec3 normal = glm::vec3(0);
		bool hit = false;

		for (uint32_t i = 0; i < node.triangleCount; i++)
		{
			const Triangle& currentTriangle = m_Triangles[m_TriangleIndices[node.leftChildOrFirstIndex + i]];
			float triangleDistance = std::numeric_limits<float>().max();
			bool triangleHit = currentTriangle.Intersect(ray.origin, ray.direction, triangleDistance);

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
			ray.objectArea = m_Area;
		}

		return hit;
	}
	else
	{
		hit |= TraverseNode(ray, node.leftChildOrFirstIndex);
		hit |= TraverseNode(ray, node.leftChildOrFirstIndex + 1);
	}

	return hit;
}

bool BVH_BLAS::IntersectAABB(glm::vec3 origin, glm::vec3 direction, float& tnear, glm::vec3 aabbMin, glm::vec3 aabbMax) const
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

float BVH_BLAS::EvaluateSAH(BVHNode& node, int axis, float position)
{
	AABB leftBox, rightBox;
	int leftCount = 0, rightCount = 0;

	for (int i = 0; i < node.triangleCount; i++)
	{
		Triangle& triangle = m_Triangles[m_TriangleIndices[i]];
		if (triangle.centroid[axis] < position)
		{
			leftCount++;
			leftBox.Grow(triangle.vertex0);
			leftBox.Grow(triangle.vertex1);
			leftBox.Grow(triangle.vertex2);
		}
		else {
			rightCount++;
			rightBox.Grow(triangle.vertex0);
			rightBox.Grow(triangle.vertex1);
			rightBox.Grow(triangle.vertex2);
		}
	}

	float cost = leftCount * leftBox.Area() + rightCount * rightBox.Area();
	return cost > 0 ? cost : std::numeric_limits<float>().max();
}

void BVH_BLAS::AddObject(const RenderObject& object, const glm::mat4& transform)
{
	const Model& model = object.m_Model;
	int faceCount = model.nfaces();

	m_Triangles.reserve(m_Triangles.size() + faceCount * 3);

	for (int faceIndex = 0; faceIndex < faceCount; faceIndex++) { 
		m_Triangles.emplace_back(Triangle(model.point(model.vert(faceIndex, 0)), model.point(model.vert(faceIndex, 1)), model.point(model.vert(faceIndex, 2))));
		
		//Add transformed triangle area
		//Triangle& faceTriangle = m_Triangles[faceIndex];
		//Triangle transformedTriangle = Triangle(glm::vec3(transform * glm::vec4(faceTriangle.vertex0, 0)),
		//										glm::vec3(transform * glm::vec4(faceTriangle.vertex1, 0)),
		//										glm::vec3(transform * glm::vec4(faceTriangle.vertex2, 0)));
		//m_Area += transformedTriangle.Area();
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
	Subdivide(rootNodeIndex, nodesUsed, useHeuristic);
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

void BVH_BLAS::Subdivide(uint32_t nodeID, uint32_t& nodesUsed, bool useHeuristic)
{
	BVHNode& node = m_BvhNodes[nodeID];

	//terminate recursion
	if (node.triangleCount <= 2) 
		return;

	int axis = -1;
	float splitPos = 0;
	//determine split axis and position
	if (useHeuristic) 
	{
		float bestCost = std::numeric_limits<float>().max();

		for (int candidateAxis = 0; candidateAxis < 3; candidateAxis++)
		{
			for (uint32_t i = 0; i < node.triangleCount; i++)
			{
				Triangle& triangle = m_Triangles[m_TriangleIndices[node.leftChildOrFirstIndex + i]];
				float candidatePosition = triangle.centroid[candidateAxis];
				float candidateCost = EvaluateSAH(node, candidateAxis, candidatePosition);
				if (candidateCost < bestCost)
				{
					bestCost = candidateCost;
					axis = candidateAxis;
					splitPos = candidatePosition;
				}
			}
		}

		//early termination if the best children have worse cost than current node
		glm::vec3 extent = node.aabbMax - node.aabbMin;
		float parentArea = extent.x * extent.y + extent.y * extent.z + extent.z * extent.x;
		float parentCost = node.triangleCount * parentArea;

		if (bestCost < parentCost)
			return;
	}
	else
	{
		axis = 0;
		glm::vec3 extent = node.aabbMax - node.aabbMin;
		if (extent.y > extent.x) axis = 1;
		if (extent.z > extent[axis]) axis = 2;
		splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
	}

	//TODO: fix actual negative axis value error
	axis = std::max(axis, 0);

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

	m_BvhNodes[leftChildIndex].leftChildOrFirstIndex = node.leftChildOrFirstIndex;
	m_BvhNodes[leftChildIndex].triangleCount = leftCount;
	m_BvhNodes[rightChildIndex].leftChildOrFirstIndex = i;
	m_BvhNodes[rightChildIndex].triangleCount = node.triangleCount - leftCount;
	node.leftChildOrFirstIndex = leftChildIndex;
	node.triangleCount = 0;
	UpdateNodeBounds(leftChildIndex);
	UpdateNodeBounds(rightChildIndex);

	//recurse
	Subdivide(leftChildIndex, nodesUsed, useHeuristic);
	Subdivide(rightChildIndex, nodesUsed, useHeuristic);
}
