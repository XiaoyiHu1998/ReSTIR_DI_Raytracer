#include "AccelerationStructures.h"

#include <iostream>
#include "Utils.h"

//=================== TLAS =========================

uint32_t TLAS::AddBLAS(const std::shared_ptr<BLAS>& BLAS)
{
	m_BLASList.push_back(BLAS);
	return m_BLASList.size() - 1;
}

void TLAS::UpdateTransform()
{
	for (int i = 0; i < m_BLASList.size(); i++)
	{
		m_BLASList[i]->UpdateTransform();
	}
}

void TLAS::Traverse(Ray& ray) const
{
	for (int i = 0; i < m_BLASList.size(); i++)
	{
		m_BLASList[i]->Traverse(ray);
	}
}

bool TLAS::IsOccluded(const Ray& ray) const
{
	int i = 0;
	bool occluded = false;
	while (!occluded && i < m_BLASList.size())
	{
		occluded |= m_BLASList[i]->IsOccluded(ray);

		i++;
	}

	return occluded;
}

//=================== BVH_BLAS =====================

void BVH_BLAS::SetObject(const std::vector<Triangle>& triangles, const Transform& transform)
{
#if defined(__AVX2__)
	m_BVH = tinybvh::BVH8_CPU();
#elif defined(__AVX__)
	m_BVH = tinybvh::BVH_SoA();
#else
	m_BVH = tinybvh::BVH4_CPU();
#endif

	m_Transform = transform;
	m_InverseTransformMatrix = transform.GetInverseTransformMatrix();
	m_Positions.reserve(triangles.size() * 3);

	for (int i = 0; i < triangles.size(); i++)
	{
		Vertex vertex0 = triangles[i].GetVertex0();
		Vertex vertex1 = triangles[i].GetVertex1();
		Vertex vertex2 = triangles[i].GetVertex2();

		m_Positions.emplace_back(vertex0.position.x, vertex0.position.y, vertex0.position.z, 0.0f);
		m_Positions.emplace_back(vertex1.position.x, vertex1.position.y, vertex1.position.z, 0.0f);
		m_Positions.emplace_back(vertex2.position.x, vertex2.position.y, vertex2.position.z, 0.0f);
	}

	m_BVH.BuildHQ(m_Positions.data(), m_Positions.size() / 3);
}

void BVH_BLAS::Traverse(Ray& ray)
{
	// Transform Ray
	glm::vec3 transformedOrigin = m_InverseTransformMatrix * glm::vec4(ray.origin, 1.0f);
	glm::vec3 transformedDirection = m_InverseTransformMatrix * glm::vec4(ray.direction, 0.0f);

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
		glm::vec3 position0 = glm::vec3(m_Positions[tinybvhRay.hit.prim * 3 + 0].x, m_Positions[tinybvhRay.hit.prim * 3 + 0].y, m_Positions[tinybvhRay.hit.prim * 3 + 0].z);
		glm::vec3 position1 = glm::vec3(m_Positions[tinybvhRay.hit.prim * 3 + 1].x, m_Positions[tinybvhRay.hit.prim * 3 + 1].y, m_Positions[tinybvhRay.hit.prim * 3 + 1].z);
		glm::vec3 position2 = glm::vec3(m_Positions[tinybvhRay.hit.prim * 3 + 2].x, m_Positions[tinybvhRay.hit.prim * 3 + 2].y, m_Positions[tinybvhRay.hit.prim * 3 + 2].z);

		// HitInfo Data
		hitInfo.distance = tinybvhRay.hit.t;
		hitInfo.position = ray.origin + ray.direction * tinybvhRay.hit.t;
		hitInfo.prevPosition = m_ToPreviousPositionMatrix * glm::vec4(hitInfo.position, 1.0f);
		hitInfo.normal = glm::normalize(glm::cross(position1 - position0, position2 - position0));
		hitInfo.prevNormal = m_ToPreviousPositionMatrix * glm::vec4(hitInfo.normal, 0.0f);
		hitInfo.traversalStepsHitBVH = traversalSteps;
		hitInfo.traversalStepsTotal += ray.hitInfo.traversalStepsTotal + traversalSteps;

		// Set new HitInfo
		ray.hitInfo = hitInfo;
	}
}

bool BVH_BLAS::IsOccluded(const Ray& ray)
{
	glm::vec3 transformedOrigin = m_InverseTransformMatrix * glm::vec4(ray.origin, 1.0f);
	glm::vec3 transformedDirection = m_InverseTransformMatrix * glm::vec4(ray.direction, 0.0f);

	tinybvh::bvhvec3 origin = tinybvh::bvhvec3(transformedOrigin.x, transformedOrigin.y, transformedOrigin.z);
	tinybvh::bvhvec3 direction = tinybvh::bvhvec3(transformedDirection.x, transformedDirection.y, transformedDirection.z);
	float maxDistance = ray.hitInfo.distance;

	return m_BVH.IsOccluded(tinybvh::Ray(origin, direction, maxDistance));
}

void BVH_BLAS::UpdateTransform()
{
	m_HasTransformed = m_PrevTransform != m_Transform;

	// Going world and object space conversion
	glm::mat4 transformMatrix = m_Transform.GetTransformMatrix(); // Object space to world space
	m_InverseTransformMatrix = glm::inverse(transformMatrix); // World to object space

	if (m_HasTransformed)
	{
		// Current position to prev position in object space
		glm::vec3 translationDelta = m_PrevTransform.translation - m_Transform.translation;
		glm::vec3 rotationDelta = m_PrevTransform.rotation - m_Transform.rotation;
		glm::vec3 scaleDelta = glm::vec3(1);

		glm::mat4 deltaMatrix = glm::translate(glm::mat4(1), translationDelta);
		deltaMatrix = glm::rotate(deltaMatrix, glm::radians(rotationDelta.z), glm::vec3(0, 0, 1));
		deltaMatrix = glm::rotate(deltaMatrix, glm::radians(rotationDelta.y), glm::vec3(0, 1, 0));
		deltaMatrix = glm::rotate(deltaMatrix, glm::radians(rotationDelta.x), glm::vec3(1, 0, 0));
		deltaMatrix = glm::scale(deltaMatrix, scaleDelta);

		// World space position -> Object Space position -> Prev object space position -> Prev world space position
		m_ToPreviousPositionMatrix = transformMatrix * deltaMatrix * m_InverseTransformMatrix;
	}
	else
	{
		m_ToPreviousPositionMatrix = glm::mat4(1);
	}

	m_PrevTransform = m_Transform;
}
