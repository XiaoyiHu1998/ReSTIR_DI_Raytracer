#ifndef ACCELERATION_STRUCTURE_HPP
#define ACCELERATION_STRUCTURE_HPP

#include "RenderObject.h"
#include "Ray.h"

struct RandomLightPoint
{
	const glm::vec3 point;
	const glm::vec3 color;
	const glm::vec3 normal;
	const float area;

	const glm::mat4 objectInverseTransform;

	RandomLightPoint(glm::vec3 point, glm::vec3 color, float area, glm::vec3 normal, glm::mat4 objectInverseTransform) :
		point{ point }, color{ color }, area{ area }, normal{ normal }, objectInverseTransform { objectInverseTransform }
	{}
};

class AccelerationStructure
{
public:
	enum class DebugMode {
		Off, On
	};

public:
	virtual ~AccelerationStructure() {}

	virtual bool Traverse(Ray& ray) = 0;
	virtual RandomLightPoint RandomTrianglePoint() const = 0;

	virtual void AddObject(const RenderObject& object) = 0;
	virtual void Build(bool useHeuristic) = 0;

	virtual void SetDebugMode(DebugMode debugMode) = 0;
	virtual int ObjectCount() = 0;
};


#endif //ACCELERATION_STRUCTURE_HPP