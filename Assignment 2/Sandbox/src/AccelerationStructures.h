#pragma once

#include "tiny_bvh.h"

#include "Include.h"
#include "Ray.h"
#include "Primitives.h"
#include "Material.h"

class BLAS
{
public:
	~BLAS() = default;

	virtual void SetObject(const std::vector<Triangle>& triangles, const glm::mat4& transform) = 0;
	virtual void Traverse(Ray& ray) = 0;

	virtual glm::mat4 GetInverseTransform() = 0;
	virtual void SetInverseTransform(glm::mat4 inverseTransform) = 0;
};


class TLAS
{
private:
	std::vector<std::shared_ptr<BLAS>> m_Scene;
public:
	TLAS() = default;
	~TLAS() = default;

	uint32_t AddBLAS(const std::shared_ptr<BLAS>& BLAS);
	void Traverse(Ray& ray) const;

	std::shared_ptr<BLAS> GetBLAS(uint32_t index) { return m_Scene[index]; }
};

// TODO: Save UV coordinates and vertex normals when constructing bvh
class BVH_BLAS : public BLAS
{
private:
	tinybvh::BVH m_BVH;
	std::vector<tinybvh::bvhvec4> m_Vertices;

	float m_Area;
	glm::mat4 m_InverseTransform;
	Material m_Material;

	// Backlog features:
	// Skybox
	// Pointlights
public:
	BVH_BLAS():
		m_BVH{ tinybvh::BVH() }, m_Vertices{ std::vector<tinybvh::bvhvec4>() }, m_InverseTransform{ glm::mat4() }, m_Area{ 0.0f },
		m_Material{ Material(Material::Type::Emissive, 1, 0, 0, glm::vec3(0.8, 0.2, 0.2), glm::vec3(0.8, 0.2, 0.2)) }
	{}

	~BVH_BLAS() = default;

	virtual void SetObject(const std::vector<Triangle>& triangles, const glm::mat4& transform) override;
	virtual void Traverse(Ray& ray) override;

	virtual glm::mat4 GetInverseTransform() { return m_InverseTransform; }
	virtual void SetInverseTransform(glm::mat4 inverseTransform) { m_InverseTransform = inverseTransform; }
};