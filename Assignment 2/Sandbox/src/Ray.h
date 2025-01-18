#pragma once

#include "Include.h"
#include "Material.h"

struct HitInfo
{
public:
	bool hit;
	float distance;
	float objectArea;

	glm::vec3 location;
	glm::vec3 normal;
	glm::vec3 tangent;

	Material material;
	Material prevMaterial;

	// Debug info
	int32_t traversalStepsHitBVH;
	int32_t traversalStepsTotal;
public:
	HitInfo():
		hit{ false }, distance{ std::numeric_limits<float>().max() }, objectArea{ 0 }, location{ glm::vec3(0) },
		traversalStepsHitBVH { 0 }, traversalStepsTotal { 0 },
		material { Material(Material::Type::Emissive, 0, glm::vec3(0), glm::vec3(0.8f, 0.2f, 0.2f), 1) },
		prevMaterial { Material(Material::Type::Emissive, 0, glm::vec3(0), glm::vec3(0.8f, 0.2f, 0.2f), 1) },
		normal { glm::vec3(0)}, tangent{ glm::vec3(0) }
	{}

	HitInfo(bool hit) : // Members should be set manually after initialization
		hit{ hit },
		material{ Material(Material::Type::Emissive, 0, glm::vec3(0), glm::vec3(0.8f, 0.2f, 0.2f),1 ) },
		prevMaterial{ Material(Material::Type::Emissive, 0, glm::vec3(0), glm::vec3(0.8f, 0.2f, 0.2f), 1) }
	{}
};

struct LightSampleInfo
{
public:
	float distance;
	float Area;
	float emissiveIntensity;
	float Probability; // Probability of finding this specific point on the light from the whole scene

	glm::vec3 location;
	glm::vec3 direction;
	glm::vec3 normal;
	glm::vec3 emissiveColor;
public:
	LightSampleInfo() :
		distance{ std::numeric_limits<float>().max() }, Area{ 0 }, direction{ glm::vec3(0) }, normal{ glm::vec3(0) }, emissiveColor{ glm::vec3(0) }, emissiveIntensity{ 0 }, Probability{ 0 }
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