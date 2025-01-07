#include "AccelerationStructures.h"

#include <iostream>
#include "Utils.h"

//=================== TLAS =========================

uint32_t TLAS::AddBLAS(const std::shared_ptr<BLAS>& BLAS)
{
	m_Scene.push_back(BLAS);
	return m_Scene.size() - 1;
}

void TLAS::Traverse(Ray& ray) const
{
	for (int i = 0; i < m_Scene.size(); i++)
	{
		m_Scene[i]->Traverse(ray);
	}
}

//=================== BVH_BLAS =====================

void BVH_BLAS::SetObject(const std::vector<Triangle>& triangles, const glm::mat4& transform, const Material& material)
{
	std::cout << "BVH_BLAS::SetObject() not properly implemented yet!" << std::endl;
	m_BVH = tinybvh::BVH();
	m_Area = 0.0f;
	m_InverseTransform = glm::inverse(transform);
	m_CumulativeArea.resize(triangles.size());
	m_Material = material;

	for (int i = 0; i < triangles.size(); i++)
	{
		m_Vertices.emplace_back(triangles[i].GetVertex0().position.x, triangles[i].GetVertex0().position.y, triangles[i].GetVertex0().position.z, 0.0f);
		m_Vertices.emplace_back(triangles[i].GetVertex1().position.x, triangles[i].GetVertex1().position.y, triangles[i].GetVertex1().position.z, 0.0f);
		m_Vertices.emplace_back(triangles[i].GetVertex2().position.x, triangles[i].GetVertex2().position.y, triangles[i].GetVertex2().position.z, 0.0f);

		m_Area += triangles[i].Area();
		m_CumulativeArea[i] = m_Area;
	}

	m_BVH.BuildHQ(m_Vertices.data(), m_Vertices.size() / 3);
}

void BVH_BLAS::Traverse(Ray& ray)
{
	// Transform Ray
	glm::vec3 transformedOrigin = m_InverseTransform * glm::vec4(ray.origin, 1.0f);
	glm::vec3 transformedDirection = m_InverseTransform * glm::vec4(ray.direction, 0.0f);

	// Intersection Test
	tinybvh::bvhvec3 origin = tinybvh::bvhvec3(transformedOrigin.x, transformedOrigin.y, transformedOrigin.z);
	tinybvh::bvhvec3 direction = tinybvh::bvhvec3(transformedDirection.x, transformedDirection.y, transformedDirection.z);
	float prevClosestHitDistance = ray.hitInfo.distance;

	tinybvh::Ray tinybvhRay = tinybvh::Ray(origin, direction, prevClosestHitDistance);
	int32_t traversalSteps = m_BVH.Intersect(tinybvhRay);

	// Hit Test
	HitInfo hitInfo(tinybvhRay.hit.t < prevClosestHitDistance);

	// Set all intersection data
	if (hitInfo.hit)
	{
		// Triangle hit
		glm::vec3 vertex0 = glm::vec3(m_Vertices[tinybvhRay.hit.prim + 0].x, m_Vertices[tinybvhRay.hit.prim + 0].y, m_Vertices[tinybvhRay.hit.prim + 0].z);
		glm::vec3 vertex1 = glm::vec3(m_Vertices[tinybvhRay.hit.prim + 1].x, m_Vertices[tinybvhRay.hit.prim + 1].y, m_Vertices[tinybvhRay.hit.prim + 1].z);
		glm::vec3 vertex2 = glm::vec3(m_Vertices[tinybvhRay.hit.prim + 2].x, m_Vertices[tinybvhRay.hit.prim + 2].y, m_Vertices[tinybvhRay.hit.prim + 2].z);
		Triangle hitTriangle = Triangle(Vertex(vertex0), Vertex(vertex1), Vertex(vertex2));

		// HitInfo Data
		hitInfo.distance = tinybvhRay.hit.t;
		hitInfo.location = ray.origin + ray.direction * tinybvhRay.hit.t;
		hitInfo.normal = hitTriangle.GetNormal();
		hitInfo.tangent = hitTriangle.GetTangent();
		hitInfo.objectArea = m_Area;
		hitInfo.prevMaterial = hitInfo.material;
		hitInfo.material = m_Material;
		hitInfo.traversalStepsHitBVH = traversalSteps;
		hitInfo.traversalStepsTotal = ray.hitInfo.traversalStepsTotal + traversalSteps;

		// Set new HitInfo
		ray.hitInfo = hitInfo;
	}
}

Triangle BVH_BLAS::GetRandomTriangle(uint32_t& seed) const
{
	// Find a random triangle using binary search.
	float randomArea = Utils::RandomFloat(seed) * m_Area;

	int left = 0; //inclusive
	int right = m_CumulativeArea.size() - 1; //exclusive

	while (left < right)
	{
		int mid = left + (right - left) / 2;
		if (m_CumulativeArea[mid] < randomArea)
		{
			left = mid + 1;
		}
		else
		{
			right = mid;
		}
	}

	uint32_t index = left * 3;

	glm::vec3 vertex0 = glm::vec3(m_Vertices[index + 0].x, m_Vertices[index + 0].y, m_Vertices[index + 0].z);
	glm::vec3 vertex1 = glm::vec3(m_Vertices[index + 1].x, m_Vertices[index + 1].y, m_Vertices[index + 1].z);
	glm::vec3 vertex2 = glm::vec3(m_Vertices[index + 2].x, m_Vertices[index + 2].y, m_Vertices[index + 2].z);

	return Triangle(Vertex(vertex0), Vertex(vertex1), Vertex(vertex2));
}
