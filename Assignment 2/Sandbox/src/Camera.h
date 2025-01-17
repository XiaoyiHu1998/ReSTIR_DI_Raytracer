#pragma once

#include "Include.h"
#include "Ray.h"
#include "Transform.h"

class Camera
{
public:
	Transform transform;
private:
	//Viewport
	float m_Width, m_Height;
	float m_VerticalFov;

	//Transform - set by user
	glm::vec3 m_TargetPosition;

	glm::vec3 m_UpDirection;
	glm::vec3 m_RightDirection;
	glm::vec3 m_BackDirection;

	//Tranform - calculated
	glm::mat4 m_TransformMatrix;
	glm::mat4 m_InverseTransformMatrix;

	float m_CameraPlaneDistance;
public:
	Camera():
		m_Width{ 1280.0f }, m_Height{ 720.0f }, m_VerticalFov{ 60 },
		transform{ Transform(glm::vec3(0, 0, 10), glm::vec3(0), glm::vec3(1)) }, m_UpDirection{ glm::vec3(0,1,0) }
	{
		UpdateCameraMatrix();
	}

	Camera(uint32_t width, uint32_t height, float verticalFov = 60):
		m_Width{ static_cast<float>(width) }, m_Height{ static_cast<float>(height) }, m_VerticalFov{ verticalFov },
		transform{ Transform(glm::vec3(0, 0, 10), glm::vec3(0), glm::vec3(1)) }, m_UpDirection{ glm::vec3(0,1,0) }
	{
		UpdateCameraMatrix();
	}

	~Camera() = default;

	Ray GetRay(uint32_t x, uint32_t y, bool random = false) const;

	float GetFOV() const				{ return m_VerticalFov; }
	float& GetFOVRef()					{ return m_VerticalFov; }
	Transform GetTransform() const		{ return transform; }
	Transform& GetTransformRef()		{ return transform; }
	glm::vec3 GetUpDirection()			{ return m_UpDirection; }
	glm::vec3 GetBackDirection()		{ return m_BackDirection; }
	glm::vec3 GetForwardDirection()		{ return -1.0f * m_BackDirection; }
	glm::vec3 GetRightDirection()		{ return m_RightDirection; }
	glm::vec3 GetLeftDirection()		{ return -1.0f * m_RightDirection; }
	glm::mat4 GetCameraMatrix()			{ return m_TransformMatrix; }
	glm::mat4 GetInverseCameraMatrix()	{ return m_InverseTransformMatrix; }
	glm::i32vec2 GetResolution()		{ return glm::i32vec2(m_Width, m_Height); }

	void SetTransform(Transform transform) { transform = transform; UpdateCameraMatrix(); }
	void SetUpDirection(glm::vec3 upDirection) { m_UpDirection = glm::normalize(upDirection); UpdateCameraMatrix(); }
	void SetResolution(uint32_t width, uint32_t height) { m_Width = static_cast<float>(width); m_Height = static_cast<float>(height); UpdateCameraMatrix(); }

	void UpdateCameraMatrix();
};