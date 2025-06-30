#pragma once

#include "tiny_bvh.h"

#include "Include.h"
#include "Ray.h"
#include "Primitives.h"
#include "Transform.h"

class BLAS
{
public:
	~BLAS() = default;

	virtual void SetObject(const std::vector<Triangle>& triangles, const Transform& transform) = 0;
	virtual void Traverse(Ray& ray) = 0;
	virtual bool IsOccluded(const Ray& ray) = 0;

	virtual void SetName(const std::string& name) = 0;
	virtual void SetTransform(const Transform& transform) = 0;

	virtual std::string& GetNameRef() = 0;
	virtual Transform GetTransform() const = 0;
	virtual Transform& GetTransformRef() = 0;
	virtual glm::mat4 GetTransformMatrix() const = 0;
	virtual glm::mat4 GetInverseTransformMatrix() const = 0;
};


class TLAS
{
private:
	std::vector<std::shared_ptr<BLAS>> m_BLASList;
public:
	TLAS() = default;
	~TLAS() = default;

	uint32_t AddBLAS(const std::shared_ptr<BLAS>& BLAS);
	void Build();

	void Traverse(Ray& ray) const;
	bool IsOccluded(const Ray& ray) const;

	uint32_t GetObjectCount() const { return m_BLASList.size(); }
	std::shared_ptr<BLAS> GetBLAS(uint32_t index) const { return m_BLASList[index]; }
};


class BVH_BLAS : public BLAS
{
private:
#if defined(__AVX2__)
	tinybvh::BVH8_CPU m_BVH;
#elif defined(__AVX__)
	tinybvh::BVH_SoA m_BVH;
#else
	tinybvh::BVH4_CPU m_BVH;
#endif
	std::vector<tinybvh::bvhvec4> m_Positions;

	std::string m_Name;
	Transform m_Transform;
	glm::mat4 m_InverseTransformMatrix;
	glm::mat4 m_TransformMatrix;
public:
	BVH_BLAS() :
		m_Name{ "" }, m_BVH{
#if defined(__AVX2__)
			tinybvh::BVH8_CPU()
#elif defined(__AVX__) && defined(USE_EXPERIMENTAL_BVH)
			tinybvh::BVH_SoA()
#else
			tinybvh::BVH4_CPU()
#endif
		}, m_Positions{ std::vector<tinybvh::bvhvec4>() },
		m_InverseTransformMatrix{ glm::mat4(1) }, m_TransformMatrix{ glm::mat4(1) }
	{}

	~BVH_BLAS() = default;

	virtual void SetObject(const std::vector<Triangle>& triangles, const Transform& transform) override;
	virtual void Traverse(Ray& ray) override;
	virtual bool IsOccluded(const Ray& ray) override;

	virtual void SetName(const std::string& name) override;
	virtual void SetTransform(const Transform& transform) override { m_Transform = transform; m_TransformMatrix = transform.GetTransformMatrix(); m_InverseTransformMatrix = glm::inverse(m_TransformMatrix); }

	virtual std::string& GetNameRef() override { return m_Name; }
	virtual Transform GetTransform() const override { return m_Transform; }
	virtual Transform& GetTransformRef() override { return m_Transform; }
	virtual glm::mat4 GetTransformMatrix() const override { return m_TransformMatrix; }
	virtual glm::mat4 GetInverseTransformMatrix() const override { return m_InverseTransformMatrix; }
};