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

	glm::vec3 Albedo;
	glm::vec3 EmissiveColor;

	float EmissiveIntensity;
	float Roughness;
public:
	Material(Material::Type materialType, float roughness, glm::vec3 albedo, glm::vec3 emmisiveColor, float emissiveStrength) :
		MaterialType{ materialType }, Roughness{ roughness }, Albedo{ albedo }, EmissiveColor{ emmisiveColor }, EmissiveIntensity{ emissiveStrength }
	{}
};