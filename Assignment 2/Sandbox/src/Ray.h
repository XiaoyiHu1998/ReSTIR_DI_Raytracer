#pragma once

#include "Include.h"

#include "Material.h"

struct HitInfo
{
public:
	bool hit;
	float distance;
	float objectArea;

	glm::vec3 hitNormal;
	glm::vec3 hitLocation;

	Material material;
	Material prevMaterial;
	//bool LastSpecular; // not necessary anymore after prevMaterial is implemented

	// Debug info
	int32_t traversalStepsHitBVH;
	int32_t traversalStepsTotal;
public:
	HitInfo():
		hit{ false }, distance{ std::numeric_limits<float>().max() }, objectArea{ 0 }, hitNormal{ glm::vec3(0) }, hitLocation{ glm::vec3(0) },
		traversalStepsHitBVH { 0 }, traversalStepsTotal { 0 }
	{}

	HitInfo(bool hit) :
		hit{ hit }
	{}
};

struct LightSampleInfo
{
public:
	float distance;
	float lightArea;

	glm::vec3 lightDirection;
	glm::vec3 lightNormal;
	glm::vec3 lightColor;
public:
	LightSampleInfo() :
		distance{ std::numeric_limits<float>().max() }, lightArea{ 0 }, lightDirection{ glm::vec3(0) }, lightNormal{ glm::vec3(0) }, lightColor{ glm::vec3(0) }
	{
	}
};

struct Ray
{
public:
	glm::vec3 origin;
	glm::vec3 direction;
	HitInfo hitInfo;
public:
	Ray(glm::vec3 origin, glm::vec3 direction, float tNear = std::numeric_limits<float>().max()) :
		origin{ origin }, direction{ direction }, hitInfo{ HitInfo() }
	{}
};