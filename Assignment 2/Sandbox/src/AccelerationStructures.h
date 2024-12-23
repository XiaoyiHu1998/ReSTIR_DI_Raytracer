//#pragma once
//
//#include "Include.h"
//#include "tiny_bvh.h"
//
//class TLAS
//{
//private:
//	std::vector<std::shared_ptr<BLAS>> m_Scene;
//public:
//	TLAS();
//	~TLAS() = default;
//
//	virtual void AddObject();
//	virtual void Build();
//
//	virtual void Traverse();
//};
//
//
//class BLAS
//{
//public:
//	~BLAS() = default;
//
//	virtual void AddObject() = 0;
//	virtual void Build() = 0;
//
//	virtual void Traverse() = 0;
//};
//
//
//class BVH_BLAS : public BLAS
//{
//private:
//	tinybvh::BVH4_CPU m_BVH;
//	tinybvh::bvhvec4 Triangles;
//
//	// Backlog features:
//	// Skybox
//	// Pointlights
//public:
//	~BVH_BLAS() = default;
//
//	virtual void AddObject();
//	virtual void Build();
//
//	virtual void Traverse();
//};