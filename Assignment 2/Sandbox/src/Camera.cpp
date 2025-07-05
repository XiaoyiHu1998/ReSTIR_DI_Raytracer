#include "Camera.h"

Ray Camera::GetRay(uint32_t x, uint32_t y) const
{
	return Ray(position, GetDirection(x, y));
}

glm::i32vec2 Camera::WorldSpaceToScreenSpace(const glm::vec3& worldPosition, uint32_t& seed) const
{
	glm::vec3 cameraToPosition = worldPosition - position;

	float topDistance = glm::dot(cameraToPosition, m_Frustrum.topNormal);
	float bottomDistance = glm::dot(cameraToPosition, m_Frustrum.bottomNormal);
	float leftDistance = glm::dot(cameraToPosition, m_Frustrum.leftNormal);
	float rightDistance = glm::dot(cameraToPosition, m_Frustrum.rightNormal);

	float u = leftDistance / (leftDistance + rightDistance);
	float v = topDistance / (topDistance + bottomDistance);

	float xOffset = Utils::RandomFloat(seed) - 0.5f;
	float yOffset = Utils::RandomFloat(seed) - 0.5f;

	return glm::i32vec2(u * m_Width + xOffset, v * m_Height + yOffset);
}

void Camera::SetResolution(uint32_t width, uint32_t height)
{
	m_Width = static_cast<float>(width);
	m_Height = static_cast<float>(height);

	m_HalfWidth = m_Width * 0.5f;
	m_HalfHeight = m_Height * 0.5f;
}

void Camera::UpdateState()
{
	UpdateCameraMatrix();
	UpdateFrustrum();
}

void Camera::UpdateCameraMatrix()
{
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));

	m_Forward = glm::vec4(position, 1.0f) + glm::vec4(0.0f, 0.0f, -1.0f, 1.0f) * rotationMatrix;
	m_Back = glm::normalize(position - m_Forward);
	m_Right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_Back));
	m_Up = glm::normalize(glm::cross(m_Back, m_Right));

	m_TransformMatrix = glm::lookAt(position, m_Forward, m_Up);
	m_CameraPlaneZ = -m_HalfHeight / (glm::tan(glm::radians(verticalFOV) * 0.5f));
}

void Camera::UpdateFrustrum()
{
	glm::vec3 origin = position;
	glm::vec3 topLeft = position + GetDirection(0, 0, false);
	glm::vec3 topRight = position + GetDirection(m_Width, 0, false);
	glm::vec3 bottomLeft = position + GetDirection(0, m_Height, false);
	glm::vec3 bottomRight = position + GetDirection(m_Width, m_Height, false);

	m_Frustrum.topNormal = Utils::TriangleNormal(origin, topRight, topLeft);
	m_Frustrum.bottomNormal = Utils::TriangleNormal(origin, bottomLeft, bottomRight);
	m_Frustrum.leftNormal = Utils::TriangleNormal(origin, topLeft, bottomLeft);
	m_Frustrum.rightNormal = Utils::TriangleNormal(origin, bottomRight, topRight);
}

glm::vec3 Camera::GetDirection(float x, float y, bool isPixel) const
{
	float delta = isPixel ? 0.5f : 0.0f;
	glm::vec3 direction = glm::vec4((x + delta) - m_HalfWidth, m_HalfHeight - (y + delta), m_CameraPlaneZ, 1.0f) * m_TransformMatrix;
	return glm::normalize(direction);
}