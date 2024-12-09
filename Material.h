#ifndef	MATERIAL_HPP
#define MATERIAL_HPP

#include "lib/glm/glm/glm.hpp"

//TODO: cleanup old stuff
struct Material
{
public:
	enum class Type
	{
		Dielectric,
		Emmisive,
		Metallic,
		Mirror
	};
public:
	Type type;

	glm::vec4 albedo_old;
	float albedo;
	glm::vec3 emittance;
	glm::vec3 diffuseColor;
	float refractiveIndex;
	float specularExponent;
public:
	Material(Type type = Type::Dielectric, glm::vec4 albedo_old = glm::vec4(0.8, 0.2, 0.2, 1.0), glm::vec3 diffuseColor = glm::vec3(0.8, 0.2, 0.2), float refractiveIndex = 0, float specularExponent = 0) :
		type{ type }, albedo{ 0.5f }, albedo_old{ albedo_old }, diffuseColor{ diffuseColor }, refractiveIndex{ refractiveIndex }, specularExponent{ specularExponent }, emittance{ glm::vec3(0.8, 0.2, 0.2) }
	{}
};


#endif //MATERIAL_HPP