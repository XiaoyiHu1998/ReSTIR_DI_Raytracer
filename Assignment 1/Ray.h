#ifndef RAY_HPP
#define RAY_HPP

#include "lib/glm/glm/glm.hpp"

#include "Material.h"

struct Ray
{
	//Ray information
	glm::vec3 origin;
	glm::vec3 direction;

	//Hit information
	float objectArea;
	float tnear;
	glm::vec3 hitLocation;
	glm::vec3 normal;
	Material material;
	Material prevHitMaterial;

	//Debug information
	int triangleIntersectionCount;
	int triangleIntersectionTestCount;
	int nodeIntersectionTestCount;
	int nodeIntersectionCount;
	int hitCount;

	Ray() :
		direction{ glm::vec3(0) }, origin{ glm::vec3(0) },
		objectArea{ 0 }, tnear{ std::numeric_limits<float>().max() }, hitLocation{ glm::vec3(0) }, normal{ glm::vec3(0) },
		material{ Material() }
	{
		triangleIntersectionCount = 0;
		triangleIntersectionTestCount = 0;
		nodeIntersectionTestCount = 0;
		nodeIntersectionCount = 0;
		hitCount = 0;
	}

	Ray(glm::vec3 origin, glm::vec3 direction) :
		origin{ origin }, direction{ direction },
		objectArea{ 0 }, tnear{ std::numeric_limits<float>().max() }, hitLocation{ glm::vec3(0) }, normal{ glm::vec3(0) },
		material{ Material() }
	{
		triangleIntersectionCount = 0;
		triangleIntersectionTestCount = 0;
		nodeIntersectionTestCount = 0;
		nodeIntersectionCount = 0;
		hitCount = 0;
	}

	Ray(glm::vec3 origin, glm::vec3 direction, float tnear) :
		origin{ origin }, direction{ direction },
		objectArea{ 0 }, tnear{ tnear }, hitLocation{ glm::vec3(0) }, normal{ glm::vec3(0) },
		material{ Material() }
	{
		triangleIntersectionCount = 0;
		triangleIntersectionTestCount = 0;
		nodeIntersectionTestCount = 0;
		nodeIntersectionCount = 0;
		hitCount = 0;
	}
};

#endif //RAY_HPP