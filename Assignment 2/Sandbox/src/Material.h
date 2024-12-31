#pragma once

#include "Include.h"

struct Material
{
public:
	enum class Type
	{
		Dielectric,
		Emissive,
		Mettalic,
		Mirror,
		Refractive
	};
private:
	Type MaterialType;

	float IOR;
	float Metallicness;
	float Roughness;

	glm::vec3 Albedo;
	glm::vec3 Emmitance;
public:
	Material(Material::Type materialType, float ior, float metallicness, float roughness, glm::vec3 albedo, glm::vec3 emmitance) :
		MaterialType{ materialType }, IOR{ ior }, Metallicness{ metallicness }, Roughness{ roughness }, Albedo{ albedo }, Emmitance{ emmitance }
	{}
};