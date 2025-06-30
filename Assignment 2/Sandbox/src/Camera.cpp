#include "Camera.h"

#include <iostream>

//void Camera::SetResolution(uint32_t width, uint32_t height)
//{
//	m_Width = static_cast<float>(width);
//	m_Height = static_cast<float>(height);
//
//	m_HalfWidth = m_Width * 0.5f;
//	m_HalfHeight = m_Height * 0.5f;
//	
//	UpdateCameraMatrix();
//}
//
//void Camera::UpdateCameraMatrix()
//{
//	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
//	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
//	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
//
//	m_TargetPosition = glm::vec4(transform.translation, 1.0f) + (glm::vec4(0.0f, 0.0f, -1.0f, 1.0f) * rotationMatrix);
//
//	m_BackDirection = glm::normalize(transform.translation - m_TargetPosition);
//	m_RightDirection = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_BackDirection));
//	m_UpDirection = glm::normalize(glm::cross(m_BackDirection, m_RightDirection));
//
//	m_TransformMatrix = glm::lookAt(transform.translation, m_TargetPosition, m_UpDirection);
//	m_InverseTransformMatrix = glm::inverse(m_TransformMatrix);
//	m_CameraPlaneZ = -m_Height / (2.0f * tan(glm::radians(m_VerticalFov) / 2.0f));
//}
//
//void Camera::UpdateFrustrum()
//{
//	m_PrevFrustrum = m_CurrentFrustrum;
//
//	glm::vec3 origin = transform.translation;
//	glm::vec3 topLeft = origin + GetDirection(0, 0);
//	glm::vec3 topRight = origin + GetDirection(m_Width, 0);
//	glm::vec3 bottomLeft = origin + GetDirection(0, m_Height);
//	glm::vec3 bottomRight = origin + GetDirection(m_Width, m_Height);
//
//	m_CurrentFrustrum.topNormal = Triangle(Vertex(origin), Vertex(topLeft), Vertex(topRight)).GetNormal();
//	m_CurrentFrustrum.bottomNormal = Triangle(Vertex(origin), Vertex(bottomRight), Vertex(bottomLeft)).GetNormal();
//	m_CurrentFrustrum.leftNormal = Triangle(Vertex(origin), Vertex(bottomLeft), Vertex(topLeft)).GetNormal();
//	m_CurrentFrustrum.rightNormal = Triangle(Vertex(origin), Vertex(topRight), Vertex(bottomRight)).GetNormal();
//}
//
//glm::vec3 Camera::GetDirection(uint32_t x, uint32_t y) const
//{
//	//TODO: double check if adding 0.5f and 0.5f of halfwidth won't result in sampling the next pixel
//	float directionX =  (static_cast<float>(x) + 0.5f) - m_HalfWidth;
//	float directionY = -(static_cast<float>(y) + 0.5f) + m_HalfHeight; // this flips the image at the same time
//
//	return glm::normalize(m_TransformMatrix * glm::vec4(directionX, directionY, m_CameraPlaneZ, 0.0f));
//}
//
//Ray Camera::GetRay(uint32_t x, uint32_t y) const
//{
//	return Ray(transform.translation, GetDirection(x, y));
//}
//
//glm::i32vec2 Camera::GetPrevFramePixelCoordinates(const glm::vec3& worldPosition) const
//{
//	glm::vec3 cameraToPoint = worldPosition - transform.translation;
//
//	// Triangle normals are already normalized
//	float topDistance = glm::abs(glm::dot(m_PrevFrustrum.topNormal, cameraToPoint));
//	float bottomDistance = glm::abs(glm::dot(m_PrevFrustrum.bottomNormal, cameraToPoint));
//	float leftDistance = glm::abs(glm::dot(m_PrevFrustrum.leftNormal, cameraToPoint));
//	float rightDistance = glm::abs(glm::dot(m_PrevFrustrum.rightNormal, cameraToPoint));
//
//	float u = leftDistance / (leftDistance + rightDistance);
//	float v = topDistance / (topDistance + bottomDistance);
//
//	return glm::i32vec2(u * m_Width, v * m_Height);
//}

Ray Camera::GetRay(uint32_t x, uint32_t y) const
{
	return Ray(position, GetDirection(x, y));
}

glm::i32vec2 Camera::WorldSpaceToScreenSpace(const glm::vec3& worldPosition) const
{
	glm::vec3 cameraToPosition = worldPosition - position;

	float topDistance = glm::dot(m_Frustrum.topNormal, cameraToPosition);
	float bottomDistance = glm::dot(m_Frustrum.bottomNormal, cameraToPosition);
	float leftDistance = glm::dot(m_Frustrum.leftNormal, cameraToPosition);
	float rightDistance = glm::dot(m_Frustrum.rightNormal, cameraToPosition);

	float u = leftDistance / (leftDistance + rightDistance);
	float v = topDistance / (topDistance + bottomDistance);

	return glm::i32vec2(u * m_Width, v * m_Height);
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
