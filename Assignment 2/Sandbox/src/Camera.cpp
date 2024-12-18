#include "Camera.h"

glm::mat4 Camera::SetCameraMatrix()
{
	m_ForwardDirection = glm::normalize(glm::vec4(1) * m_TransformMatrix);
	m_RightDirection = glm::normalize(glm::cross(m_ForwardDirection, m_UpDirection)); //TODO: check if this points left or right
	
	m_TransformMatrix = glm::lookAt(m_Position, m_TargetPosition, m_UpDirection);
	m_InverseTransformMatrix = glm::inverse(m_TransformMatrix);

	m_CameraPlaneDistance = -m_Height / (2.0f * tan(m_VerticalFov / 2.0f));
}

Ray Camera::GetRay(uint32_t x, uint32_t y, bool random = true) inline
{
	//float pixelAngle = m_VerticalFov / (float)m_Height;
	//
	//float verticalAngle = pixelAngle * (y - m_Height / 2.0f);
	//float horizontalAngle = pixelAngle * (x - m_Width / 2.0f);
	//
	//glm::mat4 rayDirectionTransform = glm::rotate(m_TransformMatrix, glm::radians(verticalAngle), m_RightDirection);
	//rayDirectionTransform = glm::rotate(rayDirectionTransform, glm::radians(horizontalAngle), m_UpDirection);
	//glm::vec3 rayDirection = rayDirectionTransform * glm::vec4(m_ForwardDirection, 1);
	//
	//return Ray(m_Position, rayDirection);

	float directionX = (x + 0.5f) - m_Width / 2.0f;
	float directionY = -(y + 0.5f) + m_Height / 2.0f; // this flips the image at the same time

	glm::vec3 rayDirection = m_InverseTransformMatrix * glm::vec4(directionX, directionY, m_CameraPlaneDistance, 1.0f);
	return Ray(m_Position, rayDirection);
}