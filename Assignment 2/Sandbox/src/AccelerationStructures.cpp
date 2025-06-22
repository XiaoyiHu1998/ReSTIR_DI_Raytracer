#include "AccelerationStructures.h"

#include <iostream>
#include "Utils.h"

//=================== TLAS =========================

uint32_t TLAS::AddBLAS(const std::shared_ptr<BLAS>& BLAS)
{
	m_BLASList.push_back(BLAS);
	return m_BLASList.size() - 1;
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
	bool occluded = false;
	for (int i = 0; i < m_BLASList.size(); i++)
	{
		occluded |= m_BLASList[i]->IsOccluded(ray);
	}

	return occluded;
}

//=================== BVH_BLAS =====================

void BVH_BLAS::SetObject(const std::vector<Triangle>& triangles, const Transform& transform, const Material& material)
{
#if defined(__AVX2__)
	m_BVH = tinybvh::BVH8_CPU();
#elif defined(__AVX__)
	m_BVH = tinybvh::BVH_SoA();
#else
	m_BVH = tinybvh::BVH4_CPU();
#endif
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
		Vertex vertex0 = triangles[i].GetVertex0();
		Vertex vertex1 = triangles[i].GetVertex1();
		Vertex vertex2 = triangles[i].GetVertex2();

		m_Positions.emplace_back(vertex0.position.x, vertex0.position.y, vertex0.position.z, 0.0f);
		m_Positions.emplace_back(vertex1.position.x, vertex1.position.y, vertex1.position.z, 0.0f);
		m_Positions.emplace_back(vertex2.position.x, vertex2.position.y, vertex2.position.z, 0.0f);

		m_Normals.push_back(vertex0.normal);
		m_Normals.push_back(vertex1.normal);
		m_Normals.push_back(vertex2.normal);
		
		m_TexCoords.push_back(vertex0.texCoord);
		m_TexCoords.push_back(vertex1.texCoord);
		m_TexCoords.push_back(vertex2.texCoord);

		// Need to account for transforms
		Triangle transformedTriangle = Triangle(Vertex(m_TransformMatrix * glm::vec4(vertex0.position, 1.0f)),
												Vertex(m_TransformMatrix * glm::vec4(vertex1.position, 1.0f)),
												Vertex(m_TransformMatrix * glm::vec4(vertex2.position, 1.0f)));

		m_Area += transformedTriangle.Area();
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

		// HitInfo Data
		hitInfo.distance = tinybvhRay.hit.t;
		hitInfo.location = ray.origin + ray.direction * tinybvhRay.hit.t;
		hitInfo.normal = glm::normalize(glm::cross(position1 - position0, position2 - position0));
		hitInfo.material = m_Material;
		hitInfo.traversalStepsHitBVH = traversalSteps;
		hitInfo.traversalStepsTotal = ray.hitInfo.traversalStepsTotal + traversalSteps;

		// Set new HitInfo
		ray.hitInfo = hitInfo;
	}
}

bool BVH_BLAS::IsOccluded(const Ray& ray)
{
	tinybvh::bvhvec3 origin = tinybvh::bvhvec3(ray.origin.x, ray.origin.y, ray.origin.z);
	tinybvh::bvhvec3 direction = tinybvh::bvhvec3(ray.direction.x, ray.direction.y, ray.direction.z);
	float maxDistance = ray.hitInfo.distance;

	return m_BVH.IsOccluded(tinybvh::Ray(origin, direction, maxDistance));
}

//=================== Debug_BLAS ===================

void Debug_BLAS::SetObject(const std::vector<Triangle>& triangles, const Transform& transform, const Material& material)
{
	m_Triangles = triangles;
	m_Area = 0.0f;
	m_CumulativeArea.resize(triangles.size());

	for (int i = 0; i < m_Triangles.size(); i++)
	{
		Vertex vertex0 = triangles[i].GetVertex0();
		Vertex vertex1 = triangles[i].GetVertex1();
		Vertex vertex2 = triangles[i].GetVertex2();

		Triangle transformedTriangle = Triangle(Vertex(m_TransformMatrix * glm::vec4(vertex0.position, 1.0f)),
												Vertex(m_TransformMatrix * glm::vec4(vertex1.position, 1.0f)),
												Vertex(m_TransformMatrix * glm::vec4(vertex2.position, 1.0f)));

		m_Area += transformedTriangle.Area();
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
			ray.hitInfo.normal = m_TransformMatrix * glm::vec4(currentTriangle.GetNormal(), 0.0f);
			ray.hitInfo.material = m_Material;
			ray.hitInfo.traversalStepsHitBVH = 1;
			ray.hitInfo.traversalStepsTotal = ray.hitInfo.traversalStepsTotal + 1;

			// Debug Values
			//ray.hitInfo.normal = currentTriangle.GetNormal();
			//ray.hitInfo.tangent = currentTriangle.GetTangent();
		}
	}
}

bool Debug_BLAS::IsOccluded(const Ray& ray)
{
	Ray occlusionRay = ray;
	Traverse(occlusionRay);

	return ray.hitInfo.hit;
}