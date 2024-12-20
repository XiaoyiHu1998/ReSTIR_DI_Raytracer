#include "Camera.h"

void Camera::SetCameraMatrix()
{	
	m_TransformMatrix = glm::lookAt(m_Position, m_TargetPosition, m_UpDirection);
	m_InverseTransformMatrix = glm::inverse(m_TransformMatrix);

	m_CameraPlaneDistance = -m_Height / (2.0f * tan(m_VerticalFov / 2.0f));
}

inline Ray Camera::GetRay(uint32_t x, uint32_t y, bool random)
{
	float directionX = (x + 0.5f) - m_Width / 2.0f;
	float directionY = -(y + 0.5f) + m_Height / 2.0f; // this flips the image at the same time

	glm::vec3 rayDirection = m_InverseTransformMatrix * glm::vec4(directionX, directionY, m_CameraPlaneDistance, 1.0f);
	return Ray(m_Position, rayDirection);
}