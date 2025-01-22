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

void Camera::UpdateFrustrum()
{
	m_PrevFrustrum = m_CurrentFrustrum;

	glm::vec3 origin = transform.translation;
	glm::vec3 topLeft = transform.translation + GetDirection(0, 0);
	glm::vec3 topRight = transform.translation + GetDirection(m_Width, 0);
	glm::vec3 bottomLeft = transform.translation + GetDirection(0, m_Height);
	glm::vec3 bottomRight = transform.translation + GetDirection(m_Width, m_Height);

	// Triangle normals are already normalized
	m_CurrentFrustrum.top = Triangle(Vertex(origin), Vertex(topLeft), Vertex(topRight)).GetNormal();
	m_CurrentFrustrum.bottom = Triangle(Vertex(origin), Vertex(bottomRight), Vertex(bottomLeft)).GetNormal();
	m_CurrentFrustrum.left = Triangle(Vertex(origin), Vertex(bottomLeft), Vertex(topLeft)).GetNormal();
	m_CurrentFrustrum.right = Triangle(Vertex(origin), Vertex(topRight), Vertex(bottomRight)).GetNormal();
}

glm::vec3 Camera::GetDirection(uint32_t x, uint32_t y) const
{
	float directionX = (x + 0.5f) - static_cast<float>(m_Width) / 2.0f;
	float directionY = -(y + 0.5f) + static_cast<float>(m_Height) / 2.0f; // this flips the image at the same time

	return glm::normalize(m_TransformMatrix * glm::vec4(directionX, directionY, m_CameraPlaneDistance, 0.0f));
}

Ray Camera::GetRay(uint32_t x, uint32_t y) const
{
	return Ray(transform.translation, GetDirection(x, y));
}

glm::i32vec2 Camera::GetPrevFramePixelCoordinates(const glm::vec3& worldPosition) const
{
	glm::vec3 cameraOffset = worldPosition - transform.translation;
	float u = glm::abs(glm::dot(m_PrevFrustrum.left, cameraOffset));
	float v = glm::abs(glm::dot(m_PrevFrustrum.top, cameraOffset));

	return glm::i32vec2(1.0f - u * m_Width, 1.0f - v * m_Height);
}