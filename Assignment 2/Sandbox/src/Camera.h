#pragma once

#include "Include.h"
#include "Ray.h"
#include "Transform.h"
#include "Primitives.h"
#include "Utils.h"

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
	//Viewport
	float m_Width, m_Height;
	float m_HalfWidth, m_HalfHeight;
	float m_CameraPlaneZ;

	//Frustrum
	FrustrumNormals m_Frustrum;

	//Transform - set by user
	glm::vec3 m_Forward;
	glm::vec3 m_Back;
	glm::vec3 m_Right;
	glm::vec3 m_Left;
	glm::vec3 m_Up;

	//Tranform - calculated
	glm::mat4 m_TransformMatrix;
	//glm::mat4 m_InverseTransformMatrix;
public:
	Camera() :
		m_Width{ 1280.0f }, m_Height{ 720.0f }, m_HalfWidth{ 640.0f }, m_HalfHeight{ 360.0f }, verticalFOV{ 60 },
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
	//glm::mat4 GetInverseCameraMatrix() { return m_InverseTransformMatrix; }

	glm::vec3 Up() { return m_Up; }
	glm::vec3 Down() { return -1.0f * m_Up; }
	glm::vec3 Forward() { return m_Forward; }
	glm::vec3 Back() { return m_Back; }
	glm::vec3 Right() { return m_Right; }
	glm::vec3 Left() { return -1.0f * m_Right; }

private:
	glm::vec3 GetDirection(float x, float y, bool isPixel = true) const;
};

//class Camera
//{
//public:
//	Transform transform;
//private:
//	//Viewport
//	float m_Width, m_Height;
//	float m_HalfWidth, m_HalfHeight;
//	float m_VerticalFov;
//
//	//Frustrum
//	FrustrumNormals m_CurrentFrustrum;
//	FrustrumNormals m_PrevFrustrum;
//
//	//Transform - set by user
//	glm::vec3 m_TargetPosition;
//
//	glm::vec3 m_UpDirection;
//	glm::vec3 m_RightDirection;
//	glm::vec3 m_BackDirection;
//
//	//Tranform - calculated
//	glm::mat4 m_TransformMatrix;
//	glm::mat4 m_InverseTransformMatrix;
//
//	float m_CameraPlaneZ;
//public:
//	Camera() :
//		m_Width{ 1280.0f }, m_Height{ 720.0f }, m_HalfWidth{ 640.0f }, m_HalfHeight{ 360.0f }, m_VerticalFov{
//		60
//	},
//		transform{ Transform(glm::vec3(0, 0, 10), glm::vec3(0), glm::vec3(1)) }, m_UpDirection{ glm::vec3(0,1,0) }
//	{
//		UpdateCameraMatrix();
//
//		// double call to init prevFrustrum
//		UpdateFrustrum();
//		UpdateFrustrum();
//	}
//
//	Camera(uint32_t width, uint32_t height, float verticalFov = 60) :
//		m_Width{ static_cast<float>(width) }, m_Height{ static_cast<float>(height) }, m_VerticalFov{ verticalFov },
//		transform{ Transform(glm::vec3(0, 0, 10), glm::vec3(0), glm::vec3(1)) }, m_UpDirection{ glm::vec3(0,1,0) }
//	{
//		m_HalfWidth = m_Width * 0.5f;
//		m_HalfHeight = m_Height * 0.5f;
//
//		UpdateCameraMatrix();
//
//		// double call to init prevFrustrum
//		UpdateFrustrum();
//		UpdateFrustrum();
//	}
//
//	~Camera() = default;
//
//	Ray GetRay(uint32_t x, uint32_t y) const;
//
//	float GetFOV() const { return m_VerticalFov; }
//	float& GetFOVRef() { return m_VerticalFov; }
//	Transform GetTransform() const { return transform; }
//	Transform& GetTransformRef() { return transform; }
//	glm::vec3 GetUpDirection() { return m_UpDirection; }
//	glm::vec3 GetBackDirection() { return m_BackDirection; }
//	glm::vec3 GetForwardDirection() { return -1.0f * m_BackDirection; }
//	glm::vec3 GetRightDirection() { return m_RightDirection; }
//	glm::vec3 GetLeftDirection() { return -1.0f * m_RightDirection; }
//	glm::mat4 GetCameraMatrix() { return m_TransformMatrix; }
//	glm::mat4 GetInverseCameraMatrix() { return m_InverseTransformMatrix; }
//	glm::i32vec2 GetResolution() { return glm::i32vec2(m_Width, m_Height); }
//	glm::i32vec2 GetPrevFramePixelCoordinates(const glm::vec3& worldPosition) const;
//
//	void SetTransform(Transform transform) { transform = transform; UpdateCameraMatrix(); }
//	void SetUpDirection(glm::vec3 upDirection) { m_UpDirection = glm::normalize(upDirection); UpdateCameraMatrix(); }
//	void SetResolution(uint32_t width, uint32_t height);
//
//	void UpdateCameraMatrix();
//	void UpdateFrustrum();
//
//private:
//	glm::vec3 GetDirection(uint32_t x, uint32_t y) const;
//};



