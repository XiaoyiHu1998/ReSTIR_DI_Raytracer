#pragma once

#include "Include.h"

struct HitInfo
{
	bool hit;
	float objectArea;
	float distance;

	glm::vec3 hitNormal;
	glm::vec3 hitLocation;

	//Material material;
	//Material prevMaterial;
	//bool LastSpecular; // not necessary anymore after prevMaterial is implemented

	HitInfo(bool hit) :
		hit{ hit }
	{}
};

struct Ray
{
	//Ray Info
	glm::vec3 origin;
	glm::vec3 direction;
	float tNear;

	//tinybvh::Ray

	Ray(glm::vec3 origin, glm::vec3 direction, float tNear = std::numeric_limits<float>().max()) :
		origin{ origin }, direction{ direction }, tNear{ tNear }
	{}

	//HitInfo GenerateHitInfo();
};