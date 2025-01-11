#pragma once

#include "Include.h"

struct Transform
{
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;

	Transform() :
		translation{ glm::vec3(0) }, rotation{ glm::vec3(0) }, scale{ glm::vec3(1) }
	{}

	Transform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale) :
		translation{ translation }, rotation{ rotation }, scale{ scale }
	{}

	glm::mat4 GetTransformMatrix() const
	{
		glm::mat4 transformMatrix = glm::scale(glm::mat4(1), scale);
		//IMPORTANT: do z -> x -> y rotation order in case of issues (unity order)
		transformMatrix = glm::rotate(transformMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
		transformMatrix = glm::rotate(transformMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
		transformMatrix = glm::rotate(transformMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
		return glm::translate(transformMatrix, translation);
	}

	glm::mat4 GetInverseTransformMatrix() const
	{
		return glm::inverse(GetTransformMatrix());
	}
};