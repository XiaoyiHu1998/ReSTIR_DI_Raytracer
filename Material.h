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

	glm::vec3 albedo;
	glm::vec3 emittedRadiance;
	glm::vec3 color;
	float refractiveIndex;
	float specularExponent;
public:
	Material(Type type = Type::Dielectric, glm::vec3 albedo = glm::vec3(0.8, 0.2, 0.2), glm::vec3 color = glm::vec3(0.8, 0.2, 0.2), float refractiveIndex = 0, float specularExponent = 0) :
		type{ type }, albedo{ albedo }, color{ color }, refractiveIndex{ refractiveIndex }, specularExponent{ specularExponent }, emittedRadiance{ glm::vec3(0.8f, 0.2f, 0.2f) }
	{}
};


#endif //MATERIAL_HPP