#pragma once

#include "Include.h"

struct Material
{
public:
	enum class Type
	{
		Non_Emissive,
		Emissive,
		Mirror,
		Refractive
	};
public:
	Type MaterialType;

	float EmissiveIntensity;
	float IOR;
	float Metallicness;
	float Roughness;

	glm::vec3 Albedo;
	glm::vec3 EmissiveColor;
public:
	Material(Material::Type materialType, float ior, float metallicness, float roughness, glm::vec3 albedo, glm::vec3 emmisiveColor, float emissiveStrength) :
		MaterialType{ materialType }, IOR{ ior }, Metallicness{ metallicness }, Roughness{ roughness }, Albedo{ albedo }, EmissiveColor{ emmisiveColor }, EmissiveIntensity{ emissiveStrength }
	{}
};