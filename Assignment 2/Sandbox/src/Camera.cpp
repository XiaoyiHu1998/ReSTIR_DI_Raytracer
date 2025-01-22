#include "Camera.h"

#include <iostream>

void Camera::SetResolution(uint32_t width, uint32_t height)
{
	m_Width = static_cast<float>(width);
	m_Height = static_cast<float>(height);
	UpdateCameraMatrix();
}

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
	glm::vec3 topLeft = origin + GetDirection(0, 0, true);
	glm::vec3 topRight = origin + GetDirection(m_Width, 0, true);
	glm::vec3 bottomLeft = origin + GetDirection(0, m_Height, true);
	glm::vec3 bottomRight = origin + GetDirection(m_Width, m_Height, true);

	m_CurrentFrustrum.top = Triangle(Vertex(origin), Vertex(topLeft), Vertex(topRight)).GetNormal();
	m_CurrentFrustrum.bottom = Triangle(Vertex(origin), Vertex(bottomRight), Vertex(bottomLeft)).GetNormal();
	m_CurrentFrustrum.left = Triangle(Vertex(origin), Vertex(bottomLeft), Vertex(topLeft)).GetNormal();
	m_CurrentFrustrum.right = Triangle(Vertex(origin), Vertex(topRight), Vertex(bottomRight)).GetNormal();
}

glm::vec3 Camera::GetDirection(uint32_t x, uint32_t y, bool translateDirection) const
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
	glm::vec3 cameraToPoint = worldPosition - transform.translation;

	// Triangle normals are already normalized
	float topDistance = glm::abs(glm::dot(m_PrevFrustrum.top, cameraToPoint));
	float bottomDistance = glm::abs(glm::dot(m_PrevFrustrum.bottom, cameraToPoint));
	float leftDistance = glm::abs(glm::dot(m_PrevFrustrum.left, cameraToPoint));
	float rightDistance = glm::abs(glm::dot(m_PrevFrustrum.right, cameraToPoint));

	float u = leftDistance / (leftDistance + rightDistance);
	float v = topDistance / (topDistance + bottomDistance);

	return glm::i32vec2(u * m_Width, v * m_Height);
}