#ifndef ACCELERATION_STRUCTURE_HPP
#define ACCELERATION_STRUCTURE_HPP

#include "RenderObject.h"
#include "Ray.h"

class AccelerationStructure
{
public:
	enum class DebugMode {
		Off, On
	};
public:
	virtual ~AccelerationStructure() {}

	virtual void SetDebugMode(DebugMode debugMode) = 0;
	virtual bool Traverse(Ray& ray) = 0;

	virtual void AddObject(const RenderObject& object) = 0;
	virtual void Build(bool useHeuristic) = 0;

	virtual int ObjectCount() = 0;
};


#endif //ACCELERATION_STRUCTURE_HPP