#pragma once

#include "Include.h"

struct Material
{
public:
	enum class Type
	{
		Non_Emissive,
		Emissive
	};
public:
	Type MaterialType;

	glm::vec3 EmissiveColor;
	float EmissiveIntensity;
public:
	Material(Material::Type materialType, glm::vec3 emmisiveColor, float emissiveIntensity) :
		MaterialType{ materialType }, EmissiveColor{ emmisiveColor }, EmissiveIntensity{ emissiveIntensity }
	{}
};