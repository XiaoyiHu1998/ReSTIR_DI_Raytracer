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

	Material material;
	Material prevMaterial;

	// Debug info
	int32_t traversalStepsHitBVH;
	int32_t traversalStepsTotal;
public:
	HitInfo():
		hit{ false }, distance{ std::numeric_limits<float>().max() }, location{ glm::vec3(0) },
		traversalStepsHitBVH { 0 }, traversalStepsTotal { 0 },
		material { Material(Material::Type::Emissive, 0, glm::vec3(0), glm::vec3(0.8f, 0.2f, 0.2f), 1) },
		prevMaterial { Material(Material::Type::Emissive, 0, glm::vec3(0), glm::vec3(0.8f, 0.2f, 0.2f), 1) },
		normal { glm::vec3(0)}
	{}

	HitInfo(bool hit) : // Members should be set manually after initialization
		hit{ hit },
		material{ Material(Material::Type::Emissive, 0, glm::vec3(0), glm::vec3(0.8f, 0.2f, 0.2f),1 ) },
		prevMaterial{ Material(Material::Type::Emissive, 0, glm::vec3(0), glm::vec3(0.8f, 0.2f, 0.2f), 1) }
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