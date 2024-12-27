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
	//void PathTrace();
	//void hdrToSdr();

	glm::vec3 Reflect(const glm::vec3& incomingDirection, const glm::vec3& normal);
	glm::vec3 Refract(const glm::vec3& incomingDirection, const glm::vec3& normal, const float eta_t, const float eta_i = 1.f);
public:
	static glm::vec4 RenderRay(Ray& ray, const TLAS& TLAS, const Sphere& sphere);
};