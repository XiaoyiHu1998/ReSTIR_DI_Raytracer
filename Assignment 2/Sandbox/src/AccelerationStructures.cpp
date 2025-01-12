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

void BVH_BLAS::SetObject(const std::vector<Triangle>& triangles, const Transform& transform, const Material& material)
{
	m_BVH = tinybvh::BVH();
	m_Area = 0.0f;
	m_CumulativeArea.resize(triangles.size());

	m_Transform = transform;
	m_InverseTransformMatrix = transform.GetInverseTransformMatrix();
	m_TransformMatrix = transform.GetTransformMatrix();

	m_Material = material;

	m_Positions.resize(0);
	m_Normals.resize(0);
	m_TexCoords.resize(0);

	m_Positions.reserve(triangles.size() * 3);
	m_Normals.reserve(triangles.size() * 3);
	m_TexCoords.reserve(triangles.size() *3);

	for (int i = 0; i < triangles.size(); i++)
	{
		m_Positions.emplace_back(triangles[i].GetVertex0().position.x, triangles[i].GetVertex0().position.y, triangles[i].GetVertex0().position.z, 0.0f);
		m_Positions.emplace_back(triangles[i].GetVertex1().position.x, triangles[i].GetVertex1().position.y, triangles[i].GetVertex1().position.z, 0.0f);
		m_Positions.emplace_back(triangles[i].GetVertex2().position.x, triangles[i].GetVertex2().position.y, triangles[i].GetVertex2().position.z, 0.0f);

		m_Normals.push_back(triangles[i].GetVertex0().normal);
		m_Normals.push_back(triangles[i].GetVertex1().normal);
		m_Normals.push_back(triangles[i].GetVertex2().normal);
		
		m_TexCoords.push_back(triangles[i].GetVertex0().texCoord);
		m_TexCoords.push_back(triangles[i].GetVertex1().texCoord);
		m_TexCoords.push_back(triangles[i].GetVertex2().texCoord);

		m_Area += triangles[i].Area();
		m_CumulativeArea[i] = m_Area;
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

		glm::vec3 normal0 = m_Normals[tinybvhRay.hit.prim * 3 + 0];
		glm::vec3 normal1 = m_Normals[tinybvhRay.hit.prim * 3 + 1];
		glm::vec3 normal2 = m_Normals[tinybvhRay.hit.prim * 3 + 2];
		
		glm::vec2 texCoord0 = m_TexCoords[tinybvhRay.hit.prim * 3 + 0];
		glm::vec2 texCoord1 = m_TexCoords[tinybvhRay.hit.prim * 3 + 1];
		glm::vec2 texCoord2 = m_TexCoords[tinybvhRay.hit.prim * 3 + 2];

		Triangle hitTriangle = Triangle(Vertex(position0, normal0, texCoord0), Vertex(position1, normal1, texCoord1), Vertex(position2, normal2, texCoord2));

		// HitInfo Data
		hitInfo.distance = tinybvhRay.hit.t;
		hitInfo.location = ray.origin + ray.direction * tinybvhRay.hit.t;
		hitInfo.normal = glm::normalize(m_TransformMatrix * glm::vec4(hitTriangle.GetNormal(), 0.0f));
		hitInfo.tangent = glm::normalize(m_TransformMatrix * glm::vec4(hitTriangle.GetTangent(), 0.0f));
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

	glm::vec3 vertex0 = glm::vec3(m_Positions[index + 0].x, m_Positions[index + 0].y, m_Positions[index + 0].z);
	glm::vec3 vertex1 = glm::vec3(m_Positions[index + 1].x, m_Positions[index + 1].y, m_Positions[index + 1].z);
	glm::vec3 vertex2 = glm::vec3(m_Positions[index + 2].x, m_Positions[index + 2].y, m_Positions[index + 2].z);

	return Triangle(Vertex(vertex0), Vertex(vertex1), Vertex(vertex2));
}

//=================== Debug_BLAS ===================

void Debug_BLAS::SetObject(const std::vector<Triangle>& triangles, const Transform& transform, const Material& material)
{
	m_Triangles = triangles;
	m_Area = 0.0f;
	m_CumulativeArea.resize(triangles.size());

	for (int i = 0; i < m_Triangles.size(); i++)
	{
		m_Area += triangles[i].Area();
		m_CumulativeArea[i] = m_Area;
	}

	m_Transform = transform;
	m_TransformMatrix = transform.GetTransformMatrix();
	m_InverseTransformMatrix = glm::inverse(m_TransformMatrix);

	m_Material = material;
}

void Debug_BLAS::Traverse(Ray& ray)
{
	glm::vec3 transformedOrigin = m_InverseTransformMatrix * glm::vec4(ray.origin, 1.0f);
	glm::vec3 transformedDirection = m_InverseTransformMatrix * glm::vec4(ray.direction, 0.0f);

	for (int i = 0; i < m_Triangles.size(); i++)
	{
		Ray currentRay = Ray(transformedOrigin, transformedDirection);
		Triangle currentTriangle = m_Triangles[i];
		if(m_Triangles[i].Intersect(currentRay) && currentRay.hitInfo.distance < ray.hitInfo.distance)
		{
			ray.hitInfo.hit = true;
			ray.hitInfo.distance = currentRay.hitInfo.distance;
			ray.hitInfo.location = ray.origin + ray.direction * currentRay.hitInfo.distance;
			ray.hitInfo.normal = m_TransformMatrix * glm::vec4(currentTriangle.GetNormal(), 1.0f);
			ray.hitInfo.tangent = m_TransformMatrix * glm::vec4(currentTriangle.GetTangent(), 1.0f);
			ray.hitInfo.objectArea = m_Area;
			ray.hitInfo.prevMaterial = ray.hitInfo.material;
			ray.hitInfo.material = m_Material;
			ray.hitInfo.traversalStepsHitBVH = 1;
			ray.hitInfo.traversalStepsTotal = ray.hitInfo.traversalStepsTotal + 1;

			// Debug Values
			//ray.hitInfo.normal = currentTriangle.GetNormal();
			//ray.hitInfo.tangent = currentTriangle.GetTangent();
		}
	}
}

Triangle Debug_BLAS::GetRandomTriangle(uint32_t& seed) const
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

	return m_Triangles[left];
}

float Debug_BLAS::GetArea() const
{
	return m_Area;
}

Material Debug_BLAS::GetMaterial() const
{
	return m_Material;
}
