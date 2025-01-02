#pragma once

#include "Glad/include/glad/glad.h"

#include "Include.h"
#include "Camera.h"
#include "Ray.h"
#include "Primitives.h"
#include "AccelerationStructures.h"

class Renderer
{
private:
	//void RayTrace();
	void PathTrace();
	//void hdrToSdr();

	glm::vec3 Reflect(const glm::vec3& incomingDirection, const glm::vec3& normal);
	glm::vec3 Refract(const glm::vec3& incomingDirection, const glm::vec3& normal, const float eta_t, const float eta_i = 1.f);
	static void Renderer::RenderKernelFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const TLAS& tlasEmmisive);
public:
	static glm::vec4 RenderRay(Ray& ray, const TLAS& tlas, const TLAS& tlasEmmisive);
	static void RenderFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, const TLAS& tlas, const TLAS& tlasEmmisive);
};