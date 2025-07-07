#pragma once

#include "tiny_bvh.h"

#include "Include.h"
#include "Ray.h"
#include "Primitives.h"
#include "Transform.h"

class BLAS
{
private:
#if defined(__AVX2__)
	tinybvh::BVH8_CPU m_BVH;
#elif defined(__AVX__)
	tinybvh::BVH_SoA m_BVH;
#else
	tinybvh::BVH4_CPU m_BVH;
#endif
	std::vector<tinybvh::bvhvec4> m_Vertices;
public:
	BLAS() :
		m_BVH{
#if defined(__AVX2__)
			tinybvh::BVH8_CPU()
#elif defined(__AVX__) && defined(USE_EXPERIMENTAL_BVH)
			tinybvh::BVH_SoA()
#else
			tinybvh::BVH4_CPU()
#endif
		}, 
		m_Vertices{ std::vector<tinybvh::bvhvec4>() }
	{}

	~BLAS() = default;

	void SetObject(const std::vector<Triangle>& triangles);
	void Refit() { m_BVH.Refit(); }

	tinybvh::BVHBase* GetBVHPointer() { return &m_BVH; }
protected:
	friend class TLAS;
	const std::vector<tinybvh::bvhvec4>& GetVertices() { return m_Vertices; }
};

class TLAS
{
private:
	// BVHs
	std::shared_ptr<tinybvh::BVH> m_TLAS;
	std::vector<std::shared_ptr<BLAS>> m_BLASList;
	std::vector<tinybvh::BLASInstance> m_BLASInstances;
	std::vector<tinybvh::BVHBase*> m_BVHPointers;

	// UI
	std::vector<std::string> m_Names;
	std::vector<Transform> m_Transforms;

	// Transforms & Reprojection
	// m_PrevTransforms is shared pointer to prevent loss of information of prev frames on copy
	std::shared_ptr<std::vector<Transform>> m_PrevTransforms; 
	std::vector<glm::mat4> m_TransformMatrices;
	std::vector<glm::mat4> m_InverseTransformMatrices;
	std::vector<glm::mat4> m_ToPreviousPositionMatrices;
public:
	TLAS() :
		m_TLAS{ std::make_shared<tinybvh::BVH>() }, m_PrevTransforms{ std::make_shared<std::vector<Transform>>() }
	{}

	~TLAS() = default;

	void Build();
	uint32_t AddBLAS(const std::shared_ptr<BLAS>& BLAS, const std::string& name, const Transform& transform);

	void Traverse(Ray& ray) const;
	bool IsOccluded(const Ray& ray) const;

	void UpdateTransform();

	uint32_t GetObjectCount() const { return m_BLASList.size(); }
	std::string& GetNameRef(uint32_t index) { return m_Names[index]; }
	Transform& GetTransformRef(uint32_t index) { return m_Transforms[index]; }
	std::shared_ptr<BLAS> GetBLAS(uint32_t index) const { return m_BLASList[index]; }
};