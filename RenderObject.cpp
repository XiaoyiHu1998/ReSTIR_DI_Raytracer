#include "RenderObject.h"

bool RenderObject::intersect(const glm::vec3& orig, const glm::vec3& dir, float& tnear)
{
	glm::mat4 invTransform = glm::inverse(m_Transform.GetTransformMatrix());
	glm::vec3 origLocal = glm::vec3(invTransform * glm::vec4(orig, 1));
	glm::vec3 dirLocal = glm::vec3(invTransform * glm::vec4(dir, 0));
	return m_Model.intersect(origLocal, dirLocal, tnear);
}