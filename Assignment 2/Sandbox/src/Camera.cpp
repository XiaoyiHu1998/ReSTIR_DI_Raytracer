#include "Camera.h"

#include <iostream>

void Camera::UpdateCameraMatrix()
{
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));

	m_TargetPosition = glm::vec4(transform.translation, 1.0f) + (glm::vec4(0.0f, 0.0f, -1.0f, 1.0f) * rotationMatrix);

	m_BackDirection = glm::normalize(transform.translation - m_TargetPosition);
	m_RightDirection = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_BackDirection));
	m_UpDirection = glm::normalize(glm::cross(m_BackDirection, m_RightDirection));

	m_TransformMatrix = glm::lookAt(transform.translation, m_TargetPosition, m_UpDirection);
	m_InverseTransformMatrix = glm::inverse(m_TransformMatrix);
	m_CameraPlaneDistance = -m_Height / (2.0f * tan(glm::radians(m_VerticalFov) / 2.0f));
}

Ray Camera::GetRay(uint32_t x, uint32_t y, bool random) const
{
	float directionX = (x + 0.5f) - static_cast<float>(m_Width) / 2.0f;
	float directionY = -(y + 0.5f) + static_cast<float>(m_Height) / 2.0f; // this flips the image at the same time

	glm::vec3 rayDirection = glm::normalize(m_TransformMatrix * glm::vec4(directionX, directionY, m_CameraPlaneDistance, 0.0f));
	return Ray(transform.translation, rayDirection);
}