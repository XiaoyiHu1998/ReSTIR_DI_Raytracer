#ifndef ACCELERATION_STRUCTURE_HPP
#define ACCELERATION_STRUCTURE_HPP

#include "RenderObject.h"

class AccelerationStructure
{
public:
	virtual ~AccelerationStructure() {}

	virtual bool RayHit(glm::vec3 origin, glm::vec3 direction, float& tnear) = 0;

	virtual void AddObject(const RenderObject& object) = 0;
	virtual void Build(bool useHeuristic) = 0;
};


#endif //ACCELERATION_STRUCTURE_HPP