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

	//void Reflect();
	//void Refract();
public:
	static glm::vec4 RenderRay(Ray& ray, const TLAS& TLAS, const Sphere& sphere);
};