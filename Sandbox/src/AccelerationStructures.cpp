#include "AccelerationStructures.h"

#include "Utils.h"

//=================== BLAS =====================

void BLAS::SetObject(const std::vector<Triangle>& triangles)
{
#if defined(__AVX2__)
	m_BVH = tinybvh::BVH8_CPU();
#elif defined(__AVX__)
	m_BVH = tinybvh::BVH_SoA();
#else
	m_BVH = tinybvh::BVH4_CPU();
#endif
	m_Vertices.reserve(triangles.size() * 3);

	for (int i = 0; i < triangles.size(); i++)
	{
		Vertex vertex0 = triangles[i].GetVertex0();
		Vertex vertex1 = triangles[i].GetVertex1();
		Vertex vertex2 = triangles[i].GetVertex2();

		m_Vertices.emplace_back(vertex0.position.x, vertex0.position.y, vertex0.position.z, 0.0f);
		m_Vertices.emplace_back(vertex1.position.x, vertex1.position.y, vertex1.position.z, 0.0f);
		m_Vertices.emplace_back(vertex2.position.x, vertex2.position.y, vertex2.position.z, 0.0f);
	}

	m_BVH.BuildHQ(m_Vertices.data(), m_Vertices.size() / 3);
}


//=================== TLAS =========================

void TLAS::Build()
{
	m_TLAS->Build(m_BLASInstances.data(), m_BLASInstances.size(), m_BVHPointers.data(), m_BLASList.size());
}

uint32_t TLAS::AddBLAS(const std::shared_ptr<BLAS>& BLAS, const std::string& name, const Transform& transform)
{
	uint32_t index = m_BLASList.size();

	m_BLASList.push_back(BLAS);
	m_Names.push_back(name);
	m_Transforms.push_back(transform);
	m_PrevTransforms->push_back(transform);

	m_TransformMatrices.push_back(transform.GetTransformMatrix());
	m_InverseTransformMatrices.push_back(transform.GetInverseTransformMatrix());
	m_ToPreviousPositionMatrices.push_back(glm::mat4(1.0f));

	m_BVHPointers.push_back(m_BLASList[index]->GetBVHPointer());
	m_BLASInstances.emplace_back(index);

	return index;
}

void TLAS::UpdateTransform()
{
	for (int i = 0; i < m_BLASList.size(); i++)
	{
		// Going world and object space conversion
		m_TransformMatrices[i] = m_Transforms[i].GetTransformMatrix(); // Object space to world space

		// Calculate ToPreviousPositionMatrix
		if ((*m_PrevTransforms)[i] != m_Transforms[i])
		{
			// Current position to prev position in object space
			glm::vec3 translationDelta = (*m_PrevTransforms)[i].translation - m_Transforms[i].translation;
			glm::vec3 rotationDelta = (*m_PrevTransforms)[i].rotation - m_Transforms[i].rotation;
			glm::vec3 scaleDelta = ((*m_PrevTransforms)[i].scale - m_Transforms[i].scale) + 1.0f;

			glm::mat4 deltaMatrix = glm::translate(glm::mat4(1), translationDelta);
			deltaMatrix = glm::rotate(deltaMatrix, glm::radians(rotationDelta.z), glm::vec3(0, 0, 1));
			deltaMatrix = glm::rotate(deltaMatrix, glm::radians(rotationDelta.y), glm::vec3(0, 1, 0));
			deltaMatrix = glm::rotate(deltaMatrix, glm::radians(rotationDelta.x), glm::vec3(1, 0, 0));
			deltaMatrix = glm::scale(deltaMatrix, scaleDelta);

			m_InverseTransformMatrices[i] = glm::inverse(m_TransformMatrices[i]); // World to object space
			// World space position -> Object Space position -> Prev object space position -> Prev world space position
			m_ToPreviousPositionMatrices[i] = m_TransformMatrices[i] * deltaMatrix * m_InverseTransformMatrices[i];
		}
		else
		{
			m_ToPreviousPositionMatrices[i] = glm::mat4(1);
		}

		(*m_PrevTransforms)[i] = m_Transforms[i];

		// Write to BLASInstance transform
		// Convert from column-major to row-major format
		const float* matrixData = (const float*)glm::value_ptr(m_TransformMatrices[i]);
		int transformIndex = 0;
		for (int column = 0; column < 4; column++)
		{
			for (int item = 0; item < 4; item++)
			{
				int index = column + item * 4;
				m_BLASInstances[i].transform[transformIndex] = matrixData[index];
				transformIndex++;
			}
		}
	}

}

void TLAS::Traverse(Ray& ray) const
{
	// Intersection Test
	tinybvh::bvhvec3 origin = tinybvh::bvhvec3(ray.origin.x, ray.origin.y, ray.origin.z);
	tinybvh::bvhvec3 direction = tinybvh::bvhvec3(ray.direction.x, ray.direction.y, ray.direction.z);
	float prevClosestHitDistance = ray.hitInfo.distance;

	tinybvh::Ray tinybvhRay = tinybvh::Ray(origin, direction, prevClosestHitDistance);
	int32_t traversalSteps = m_TLAS->Intersect(tinybvhRay);

	// Hit Test
	HitInfo hitInfo(tinybvhRay.hit.t < prevClosestHitDistance);

	// Set all intersection data
	if (hitInfo.hit)
	{
#if INST_IDX_BITS == 32
		uint32_t vertexIndex = tinybvhRay.hit.prim;
		uint32_t instanceIndex = (uint32_t)tinybvhRay.hit.inst;
#else
		uint32_t vertexIndex = tinybvhRay.hit.prim & PRIM_IDX_MASK;
		uint32_t instanceIndex = (uint32_t)tinybvhRay.hit.prim >> INST_IDX_SHFT;
#endif
		int blasIndex = m_BLASInstances[instanceIndex].blasIdx;
		const std::vector<tinybvh::bvhvec4>& vertices = m_BLASList[blasIndex]->GetVertices();

		// Triangle hit
		glm::vec3 position0 = glm::vec3(vertices[vertexIndex * 3 + 0].x, vertices[vertexIndex * 3 + 0].y, vertices[vertexIndex * 3 + 0].z);
		glm::vec3 position1 = glm::vec3(vertices[vertexIndex * 3 + 1].x, vertices[vertexIndex * 3 + 1].y, vertices[vertexIndex * 3 + 1].z);
		glm::vec3 position2 = glm::vec3(vertices[vertexIndex * 3 + 2].x, vertices[vertexIndex * 3 + 2].y, vertices[vertexIndex * 3 + 2].z);

		glm::mat4 toPreviousPosition = m_ToPreviousPositionMatrices[blasIndex];

		// HitInfo Data
		hitInfo.distance = tinybvhRay.hit.t;
		hitInfo.position = ray.origin + ray.direction * tinybvhRay.hit.t;
		hitInfo.prevPosition = toPreviousPosition * glm::vec4(hitInfo.position, 1.0f);
		hitInfo.normal = glm::normalize(m_TransformMatrices[blasIndex] * glm::vec4(Utils::TriangleNormal(position0, position1, position2), 0.0f));
		hitInfo.prevNormal = glm::normalize(toPreviousPosition * glm::vec4(hitInfo.normal, 0.0f));
		hitInfo.traversalStepsHitBVH = traversalSteps;
		hitInfo.traversalStepsTotal += ray.hitInfo.traversalStepsTotal + traversalSteps;

		// Set new HitInfo
		ray.hitInfo = hitInfo;
	}
}

bool TLAS::IsOccluded(const Ray& ray) const
{
	tinybvh::bvhvec3 origin = tinybvh::bvhvec3(ray.origin.x, ray.origin.y, ray.origin.z);
	tinybvh::bvhvec3 direction = tinybvh::bvhvec3(ray.direction.x, ray.direction.y, ray.direction.z);
	float maxDistance = ray.hitInfo.distance;

	return m_TLAS->IsOccluded(tinybvh::Ray(origin, direction, maxDistance));
}