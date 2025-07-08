#pragma once

#include "Include.h"
#include "Ray.h"

class PointLight
{
public:
	glm::vec3 position;
	glm::vec3 emmission;
public:
	PointLight():
		position{glm::vec3(std::numeric_limits<float>().infinity())}, emmission{glm::vec3(1.0f, 0.0f, 1.0f)}
	{}

	PointLight(const glm::vec3& position, const glm::vec3& color, float intensity):
		position{ position }, emmission{ color * intensity }
	{}
};