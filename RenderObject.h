#ifndef OBJECT_CPP
#define OBJECT_CPP

#include "model.h"
#include "glm/glm.hpp"

struct Transform {
	glm::vec3 translation, rotation, scale;
	glm::mat4 transformMatrix;

	Transform(glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale) :
		translation{ translation }, rotation{ rotation }, scale{ scale }, transformMatrix{glm::mat4(1)}
	{}

	glm::mat4 GetTransformMatrix()
	{
		transformMatrix = glm::mat4(1);
		transformMatrix = glm::scale(transformMatrix, scale);
		transformMatrix = glm::rotate(transformMatrix, rotation[0], glm::vec3(1,0,0));
		transformMatrix = glm::rotate(transformMatrix, rotation[1], glm::vec3(0,1,0));
		transformMatrix = glm::rotate(transformMatrix, rotation[2], glm::vec3(0,0,1));
		transformMatrix = glm::translate(transformMatrix, translation);

		return transformMatrix;
	}
};

class RenderObject {
public:
	Model m_Model;
	Transform m_Transform;

	RenderObject(Model model, Transform transform) :
		m_Model{ model }, m_Transform{ transform }
	{}
};


#endif //OBJECT_CPP