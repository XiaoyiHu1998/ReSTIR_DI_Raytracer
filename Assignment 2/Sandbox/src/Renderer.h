#pragma once

#include "Glad/include/glad/glad.h"

#include "Include.h"
#include "Camera.h"
#include "Ray.h"

class Renderer
{
private:
	//void RayTrace();
	//void PathTrace();
	//void hdrToSdr();

	//void Reflect();
	//void Refract();
public:
	static uint8_t RenderFrame(Ray& ray);
};