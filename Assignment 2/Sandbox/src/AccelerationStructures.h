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
	virtual bool IsOccluded(const Ray& ray) = 0;

	virtual void SetTransform(const Transform& transform) = 0;
	virtual void SetName(const std::string& name) = 0;

	virtual float GetArea() const = 0;
	virtual std::string GetName() const = 0;
	virtual std::string& GetNameRef() = 0;
	virtual Material GetMaterial() const = 0;
	virtual Material& GetMaterialRef() = 0;
	virtual Transform GetTransform() const = 0;
	virtual Transform& GetTransformRef() = 0;
	virtual glm::mat4 GetTransformMatrix() const = 0;
	virtual glm::mat4 GetInverseTransformMatrix() const = 0;
	virtual Triangle GetRandomTriangle(float& triangleChanceOut, uint32_t& seed) const = 0;
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
	bool IsOccluded(const Ray& ray) const;

	uint32_t GetObjectCount() const { return m_Scene.size(); }
	std::shared_ptr<BLAS> GetBLAS(uint32_t index) const { return m_Scene[index]; }
};


class BVH_BLAS : public BLAS
{
private:
	std::string m_Name;

	tinybvh::BVH4_CPU m_BVH;
	std::vector<tinybvh::bvhvec4> m_Positions;
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
		m_Name{ "" }, m_BVH{tinybvh::BVH4_CPU()}, m_Positions{std::vector<tinybvh::bvhvec4>()}, m_Normals{std::vector<glm::vec3>()}, m_TexCoords{std::vector<glm::vec2>()},
		m_InverseTransformMatrix{ glm::mat4(1) }, m_TransformMatrix{ glm::mat4(1) }, m_Area{ 0.0f },
		m_Material{ Material(Material::Type::Emissive, 0, glm::vec3(0.8, 0.2, 0.2), glm::vec3(0.8, 0.2, 0.2), 1.0f) }
	{}

	~BVH_BLAS() = default;

	virtual void SetObject(const std::vector<Triangle>& triangles, const Transform& transform, const Material& material) override;
	virtual void Traverse(Ray& ray) override;
	virtual bool IsOccluded(const Ray& ray) override;

	virtual void SetTransform(const Transform& transform) override { m_Transform = transform; m_TransformMatrix = transform.GetTransformMatrix(); m_InverseTransformMatrix = glm::inverse(m_TransformMatrix); }
	virtual void SetName(const std::string& name) override { m_Name = name; }

	virtual float GetArea() const override { return m_Area; }
	virtual std::string GetName() const override { return m_Name; }
	virtual std::string& GetNameRef() override { return m_Name; }
	virtual Material GetMaterial() const override { return m_Material; }
	virtual Material& GetMaterialRef() override { return m_Material; }
	virtual Transform GetTransform() const override { return m_Transform; }
	virtual Transform& GetTransformRef() override { return m_Transform; }
	virtual glm::mat4 GetTransformMatrix() const override { return m_TransformMatrix; }
	virtual glm::mat4 GetInverseTransformMatrix() const override { return m_InverseTransformMatrix; }
	virtual Triangle GetRandomTriangle(float& triangleChanceOut, uint32_t& seed) const override;
};


class Debug_BLAS : public BLAS
{
private:
	std::string m_Name;

	std::vector<Triangle> m_Triangles;
	std::vector<float> m_CumulativeArea;

	float m_Area;
	Transform m_Transform;
	glm::mat4 m_InverseTransformMatrix;
	glm::mat4 m_TransformMatrix;
	Material m_Material;
public:
	Debug_BLAS():
		m_Name{ "" }, m_Triangles{ std::vector<Triangle>() },m_CumulativeArea{ std::vector<float>() }, m_Area{ 0.0f }, m_Transform { Transform() },
		m_InverseTransformMatrix{ glm::mat4(1) }, m_TransformMatrix{ glm::mat4(1) }, m_Material { Material(Material::Type::Emissive, 0, glm::vec3(0.8, 0.2, 0.2), glm::vec3(0.8, 0.2, 0.2), 1.0f) }
	{}

	~Debug_BLAS() = default;

	virtual void SetObject(const std::vector<Triangle>& triangles, const Transform& transform, const Material& material) override;
	virtual void Traverse(Ray& ray) override;
	virtual bool IsOccluded(const Ray& ray) override;

	virtual void SetTransform(const Transform& transform) override { m_Transform = transform; m_TransformMatrix = transform.GetTransformMatrix(); m_InverseTransformMatrix = glm::inverse(m_TransformMatrix); }
	virtual void SetName(const std::string& name) override { m_Name = name; }
	
	virtual float GetArea() const override { return m_Area; }
	virtual std::string GetName() const override { return m_Name; }
	virtual std::string& GetNameRef() override { return m_Name; }
	virtual Material GetMaterial() const override { return m_Material; }
	virtual Material& GetMaterialRef() override { return m_Material; }
	virtual Transform GetTransform() const override { return m_Transform; }
	virtual Transform& GetTransformRef() override { return m_Transform; }
	virtual glm::mat4 GetTransformMatrix() const override { return m_TransformMatrix; }
	virtual glm::mat4 GetInverseTransformMatrix() const override { return m_InverseTransformMatrix; }
	virtual Triangle GetRandomTriangle(float& triangleChanceOut, uint32_t& seed) const override;
};