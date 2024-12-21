#pragma once

class AccelerationStructure
{
public:
	~AccelerationStructure() = default;

	virtual void AddObject() = 0;
	virtual void Build() = 0;

	virtual void Traverse() = 0;
};