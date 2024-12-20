#pragma once

#include "Include.h"
#include "Ray.h"

class Camera
{
private:
	//Viewport
	uint32_t m_Width, m_Height;
	float m_VerticalFov;

	//Transform - set by user
	glm::vec3 m_Position;
	glm::vec3 m_TargetPosition;
	glm::vec3 m_UpDirection;

	//Tranform - calculated
	glm::mat4 m_TransformMatrix;
	glm::mat4 m_InverseTransformMatrix;

	float m_CameraPlaneDistance;

	//std::vector<Ray> m_Rays;
private:
	void SetCameraMatrix();
public:
	Camera(uint32_t width, uint32_t height, float verticalFov):
		m_Width{width}, m_Height{height}, m_VerticalFov{verticalFov},
		m_Position{ glm::vec3(0) }, m_TargetPosition{ glm::vec3(0,0,-1) }, m_UpDirection{ glm::vec3(0,1,0) }
		//, m_Rays{ std::vector<Ray>() }
	{
		SetCameraMatrix();
	}

	~Camera() = default;

	inline Ray GetRay(uint32_t x, uint32_t y, bool random = false);

	glm::vec3 GetPosition()		  { return m_Position; }
	glm::vec3 GetTargetPosition() { return m_TargetPosition; }
	glm::vec3 GetUpDirection()	  { return m_UpDirection; }
	glm::mat4 GetCameraMatrix()	  { return m_TransformMatrix; }

	void SetPosition(glm::vec3 position) { m_Position = position; SetCameraMatrix(); }
	void SetTargetPosition(glm::vec3 targetPosition) { m_TargetPosition = targetPosition; SetCameraMatrix(); }
	void SetUpDirection(glm::vec3 upDirection) { m_UpDirection = glm::normalize(upDirection); SetCameraMatrix(); }
};