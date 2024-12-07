#ifndef ACCELERATION_STRUCTURE_HPP
#define ACCELERATION_STRUCTURE_HPP

#include "RenderObject.h"

class AccelerationStructure
{
public:
	enum class DebugMode {
		Off, IntersectionCount
	};
public:
	virtual ~AccelerationStructure() {}

	virtual void SetDebugMode(DebugMode debugMode) = 0;
	virtual bool Traverse(const glm::vec3& origin, const glm::vec3& direction, float& tnear) = 0;

	virtual void AddObject(const RenderObject& object) = 0;
	virtual void Build(bool useHeuristic) = 0;

	virtual int ObjectCount() = 0;
};


#endif //ACCELERATION_STRUCTURE_HPP