#ifndef	MATERIAL_HPP
#define MATERIAL_HPP

#include "lib/glm/glm/glm.hpp"

struct Material
{
public:
	enum class Type
	{
		Diffuse,
		Glossy,
		Mirror,
		Glass,
		Emmisive
	};
public:
	Type type;

	glm::vec4 albedo;
	glm::vec3 diffuseColor;
	float refractiveIndex;
	float specularExponent;
public:
	Material(Type type = Type::Diffuse, glm::vec4 albedo = glm::vec4(0.8, 0.2, 0.2, 1.0), glm::vec3 diffuseColor = glm::vec3(0.8, 0.2, 0.2), float refractiveIndex = 0, float specularExponent = 0) :
		type{ type }, albedo{albedo}, diffuseColor{ diffuseColor }, refractiveIndex{ refractiveIndex }, specularExponent{ specularExponent }
	{}
};


#endif //MATERIAL_HPP