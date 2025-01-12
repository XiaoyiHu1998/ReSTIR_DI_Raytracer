#pragma once

#include "tiny_bvh.h"

#include "Include.h"
#include "Ray.h"
#include "Primitives.h"
#include "Material.h"
#include "Transform.h"

class BLAS
{
public:
	~BLAS() = default;

	virtual void SetObject(const std::vector<Triangle>& triangles, const Transform& transform, const Material& material) = 0;
	virtual void Traverse(Ray& ray) = 0;

	virtual glm::mat4 GetInverseTransform() = 0;
	virtual void SetTransform(const Transform& transform) = 0;

	virtual Triangle GetRandomTriangle(uint32_t& seed) const = 0;
	virtual float GetArea() const = 0;
	virtual Material GetMaterial() const = 0;
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

	uint32_t GetObjectCount() const { return m_Scene.size(); }
	std::shared_ptr<BLAS> GetBLAS(uint32_t index) const { return m_Scene[index]; }
};


class BVH_BLAS : public BLAS
{
private:
	tinybvh::BVH m_BVH;
	std::vector<tinybvh::bvhvec4> m_Vertices;
	std::vector<glm::vec3> m_Normals;
	std::vector<glm::vec2> m_TexCoords;
	std::vector<float> m_CumulativeArea;

	float m_Area;
	Transform m_Transform;
	glm::mat4 m_InverseTransformMatrix;
	glm::mat4 m_TransformMatrix;
	Material m_Material;

	// Backlog features:
	// Skybox
	// Pointlights
public:
	BVH_BLAS():
		m_BVH{ tinybvh::BVH() }, m_Vertices{ std::vector<tinybvh::bvhvec4>() }, m_Normals{ std::vector<glm::vec3>() }, m_TexCoords{ std::vector<glm::vec2>() },
		m_InverseTransformMatrix{ glm::mat4(1) }, m_TransformMatrix{ glm::mat4(1) }, m_Area{ 0.0f },
		m_Material{ Material(Material::Type::Emissive, 1, 0, 0, glm::vec3(0.8, 0.2, 0.2), glm::vec3(0.8, 0.2, 0.2)) }
	{}

	~BVH_BLAS() = default;

	virtual void SetObject(const std::vector<Triangle>& triangles, const Transform& transform, const Material& material) override;
	virtual void Traverse(Ray& ray) override;

	virtual glm::mat4 GetInverseTransform() { return m_InverseTransformMatrix; }
	virtual void SetTransform(const Transform& transform) override { m_Transform = transform; m_TransformMatrix = transform.GetTransformMatrix(); m_InverseTransformMatrix = transform.GetInverseTransformMatrix(); }

	virtual Triangle GetRandomTriangle(uint32_t& seed) const override;
	virtual float GetArea() const override { return m_Area; }
	virtual Material GetMaterial() const override { return m_Material; }
};