#include "AccelerationStructures.h"

#include <iostream>

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

void BVH_BLAS::SetObject(const std::vector<Triangle>& triangles, const glm::mat4& transform)
{
	std::cout << "BVH_BLAS::SetObject() not properly implemented yet!" << std::endl;
	m_BVH = tinybvh::BVH();
	m_Area = 0.0f;
	m_InverseTransform = glm::inverse(transform);

	for (int i = 0; i < triangles.size(); i++)
	{
		m_Triangles.emplace_back(tinybvh::bvhvec4(triangles[i].GetVertex0().position.x, triangles[i].GetVertex0().position.y, triangles[i].GetVertex0().position.z, 0.0f));
		m_Triangles.emplace_back(tinybvh::bvhvec4(triangles[i].GetVertex1().position.x, triangles[i].GetVertex1().position.y, triangles[i].GetVertex1().position.z, 0.0f));
		m_Triangles.emplace_back(tinybvh::bvhvec4(triangles[i].GetVertex2().position.x, triangles[i].GetVertex2().position.y, triangles[i].GetVertex2().position.z, 0.0f));

		m_Area += triangles[i].Area();
	}

	m_BVH.Build(m_Triangles.data(), m_Triangles.size());
}

void BVH_BLAS::Traverse(Ray& ray)
{
	// Transform Ray
	glm::vec3 transformedOrigin = m_InverseTransform * glm::vec4(ray.origin, 1.0f);
	glm::vec3 transformedDirection = m_InverseTransform * glm::vec4(ray.direction, 0.0f);

	// Intersection Test
	tinybvh::bvhvec3 origin = tinybvh::bvhvec3(transformedOrigin.x, transformedOrigin.y, transformedOrigin.z);
	tinybvh::bvhvec3 direction = tinybvh::bvhvec3(transformedDirection.x, transformedDirection.y, transformedDirection.z);
	//float oldHitDistance = ray.hitInfo.distance;
	float oldHitDistance = std::numeric_limits<float>().max();

	tinybvh::Ray tinybvhRay = tinybvh::Ray(origin, direction, oldHitDistance);
	int32_t traversalSteps = m_BVH.Intersect(tinybvhRay);

	// Hit Test
	HitInfo hitInfo(tinybvhRay.hit.t < oldHitDistance);

	// Set all intersection data
	if (hitInfo.hit)
	{
		// Triangle hit
		glm::vec3 vertex0 = glm::vec3(m_Triangles[tinybvhRay.hit.prim + 0].x, m_Triangles[tinybvhRay.hit.prim + 0].y, m_Triangles[tinybvhRay.hit.prim + 0].z);
		glm::vec3 vertex1 = glm::vec3(m_Triangles[tinybvhRay.hit.prim + 1].x, m_Triangles[tinybvhRay.hit.prim + 1].y, m_Triangles[tinybvhRay.hit.prim + 1].z);
		glm::vec3 vertex2 = glm::vec3(m_Triangles[tinybvhRay.hit.prim + 2].x, m_Triangles[tinybvhRay.hit.prim + 2].y, m_Triangles[tinybvhRay.hit.prim + 2].z);
		Triangle hitTriangle = Triangle(Vertex(vertex0), Vertex(vertex1), Vertex(vertex2));

		// HitInfo Data
		hitInfo.distance = tinybvhRay.hit.t;
		hitInfo.hitLocation = ray.origin + ray.direction * hitInfo.distance;
		hitInfo.hitNormal = hitTriangle.GetNormal();
		hitInfo.objectArea = m_Area;
		// hitInfo.prevMaterial = hitInfo.material;
		// hitInfo.material = m_Material;
		hitInfo.traversalStepsHitBVH = traversalSteps;
		hitInfo.traversalStepsTotal = ray.hitInfo.traversalStepsTotal + traversalSteps;

		// Set new HitInfo
		ray.hitInfo = hitInfo;
	}
}
