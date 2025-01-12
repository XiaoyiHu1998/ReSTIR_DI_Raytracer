#pragma once

#include <thread>

#include "Glad/include/glad/glad.h"

#include "Include.h"
#include "Camera.h"
#include "Ray.h"
#include "Primitives.h"
#include "AccelerationStructures.h"

class Renderer
{
public:
	struct Settings
	{
		uint32_t ThreadCount = std::thread::hardware_concurrency() - 2;

		uint32_t RenderResolutionWidth = 640;
		uint32_t RenderResolutionHeight = 480;
		uint32_t RenderingKernelSize = 16;
		uint32_t SamplesPerPixel = 1;

		uint32_t MaxRayDepth = 8;

		bool RenderNormals = true;
	};
private:
	Settings m_Settings;
	float m_LastFrameTime;

	glm::vec3 Reflect(const glm::vec3& incomingDirection, const glm::vec3& normal);
	glm::vec3 Refract(const glm::vec3& incomingDirection, const glm::vec3& normal, const float eta_t, const float eta_i = 1.f);
	glm::vec3 RandomPointOnHemisphere(const glm::vec3& normal, uint32_t& seed);
	glm::vec3 CosineSampleHemisphere(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& tangent, uint32_t& seed);

	void Renderer::RenderKernelFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const TLAS& tlasEmmisive, uint32_t seed);
public:
	Renderer() :
		m_LastFrameTime{ 0.0f }
	{}

	glm::vec4 RenderRay(Ray& ray, const TLAS& tlas, const TLAS& tlasEmmisive, uint32_t& seed);
	void RenderFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, const TLAS& tlas, const TLAS& tlasEmmisive);
	LightSampleInfo SampleRandomLight(const Ray& ray, const TLAS& tlasEmmisive, uint32_t& seed);

	Settings& GetSettings()  { return m_Settings; }
	float GetLastFrameTime() { return m_LastFrameTime; }
};