#pragma once

#include "Include.h"
#include "Material.h"

struct HitInfo
{
public:
	bool hit;
	float distance;

	glm::vec3 location;
	glm::vec3 normal;

	// Debug info
	int32_t traversalStepsHitBVH;
	int32_t traversalStepsTotal;
public:
	HitInfo():
		hit{ false }, distance{ std::numeric_limits<float>().max() }, location{ glm::vec3(0) },
		traversalStepsHitBVH { 0 }, traversalStepsTotal { 0 },
		normal { glm::vec3(0)}
	{}

	HitInfo(bool hit) : // Members should be set manually after initialization
		hit{ hit }
	{}
};

struct Ray
{
public:
	glm::vec3 origin;
	glm::vec3 direction;
	HitInfo hitInfo;
public:
	Ray() = default;

	Ray(glm::vec3 origin, glm::vec3 direction, float tNear = std::numeric_limits<float>().max()) :
		origin{ origin }, direction{ direction }, hitInfo{ HitInfo() }
	{
		hitInfo.distance = tNear;
	}
};