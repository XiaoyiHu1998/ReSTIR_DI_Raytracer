#pragma once

#include "Include.h"
#include "Ray.h"
#include "Transform.h"
#include "Utils.h"

// Used for temporal reprojection
struct FrustrumNormals
{
	glm::vec3 topNormal;
	glm::vec3 bottomNormal;
	glm::vec3 leftNormal;
	glm::vec3 rightNormal;
};

class Camera
{
public:
	glm::vec3 position;
	glm::vec3 rotation;
	float verticalFOV;
private:
	// Viewport
	float m_Width, m_Height;
	float m_HalfWidth, m_HalfHeight;
	float m_CameraPlaneZ;

	// Frustrum
	FrustrumNormals m_Frustrum;

	// Transform
	glm::vec3 m_Forward;
	glm::vec3 m_Back;
	glm::vec3 m_Right;
	glm::vec3 m_Left;
	glm::vec3 m_Up;
	glm::mat4 m_TransformMatrix;
public:
	Camera() :
		m_Width{ 1280.0f }, m_Height{ 720.0f }, m_HalfWidth{ 640.0f }, m_HalfHeight{ 360.0f }, verticalFOV{ 60.0f },
		position{ glm::vec3(0, 0, 10) }, rotation{ glm::vec3(0) }, m_Up{ glm::vec3(0, 1, 0) }
	{
		m_HalfWidth = m_Width * 0.5f;
		m_HalfHeight = m_Height * 0.5f;
		UpdateCameraMatrix();
		UpdateFrustrum();
	}

	Camera(uint32_t width, uint32_t height, float verticalFov = 60) :
		m_Width{ static_cast<float>(width) }, m_Height{ static_cast<float>(height) }, verticalFOV{ verticalFov },
		position{ glm::vec3(0, 0, 10) }, rotation{ glm::vec3(0) }, m_Up{ glm::vec3(0,1,0) }
	{
		m_HalfWidth = m_Width * 0.5f;
		m_HalfHeight = m_Height * 0.5f;
		UpdateCameraMatrix();
		UpdateFrustrum();
	}

	~Camera() = default;

	Ray GetRay(uint32_t x, uint32_t y) const;
	glm::i32vec2 WorldSpaceToScreenSpace(const glm::vec3& worldPosition, uint32_t& seed) const;

	void SetResolution(uint32_t width, uint32_t height);

	void UpdateState();
	void UpdateCameraMatrix();
	void UpdateFrustrum();

	glm::i32vec2 GetResolution() { return glm::i32vec2(m_Width, m_Height); }
	glm::mat4 GetCameraMatrix() { return m_TransformMatrix; }

	glm::vec3 Up() { return m_Up; }
	glm::vec3 Down() { return -1.0f * m_Up; }
	glm::vec3 Forward() { return m_Forward; }
	glm::vec3 Back() { return m_Back; }
	glm::vec3 Right() { return m_Right; }
	glm::vec3 Left() { return -1.0f * m_Right; }

private:
	glm::vec3 GetDirection(float x, float y, bool isPixel = true) const;
};