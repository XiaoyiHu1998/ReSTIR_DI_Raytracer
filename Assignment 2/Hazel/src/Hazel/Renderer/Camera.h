#pragma once

#include "glm/glm.hpp"

namespace Hazel {

	class PerspectiveCamera
	{
	public:
		struct Transform
		{
			glm::vec3 translation;
			glm::vec3 rotation;
		
			Transform(glm::vec3 translation = glm::vec3(0.0f), glm::vec3 rotation = glm::vec3(0.0f)):
				translation{ translation }, rotation{ rotation }
			{}
		};

		enum class ControlMode
		{
			FirstPerson,
			TargetLock,
		};
	public:
		PerspectiveCamera(float fov, float aspectRatio, float clipDistanceNear, float clipDistanceFar, PerspectiveCamera::ControlMode controlMode = PerspectiveCamera::ControlMode::FirstPerson);
		~PerspectiveCamera() {}

		const glm::mat4& GetViewMatrix() { RecalculateViewMatrix();  return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix() { RecalculateProjectionMatrix(); return m_ProjectionMatrix; }
		const glm::mat4& GetViewProjectionMatrix() { RecalculateViewProjectionMatrix(); return m_ViewProjectionMatrix; }

		const glm::vec3& GetUpDirection() { RecalculateDirections(); return m_UpDirection; }
		const glm::vec3& GetRightDirection() {RecalculateDirections(); return m_RightDirection; }
		const glm::vec3& GetForwardDirection() {RecalculateDirections(); return m_BackDirection * -1.0f; }
	public:
		float fov;
		float aspectRatio;
		float clipDistanceNear;
		float clipDistanceFar;

		PerspectiveCamera::ControlMode controlMode;
		PerspectiveCamera::Transform transform;
		glm::vec3 targetLocation;
	private:
		void RecalculateDirections();
		void RecalculateViewMatrix();
		void RecalculateProjectionMatrix();
		void RecalculateViewProjectionMatrix();
	private:
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewProjectionMatrix;

		glm::vec3 m_BackDirection;
		glm::vec3 m_RightDirection;
		glm::vec3 m_UpDirection;
		glm::vec3 m_TargetLocation;
	};


	class OrthographicCamera
	{
	public:
		OrthographicCamera(float left, float right, float top, float bottom);
		~OrthographicCamera() {}

		const glm::vec3& GetPosition() const { return m_Position; }
		void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateViewProjectionMatrix(); }

		float GetRotation() const { return m_Rotation; }
		void SetRotation(float rotation) { m_Rotation = rotation; RecalculateViewProjectionMatrix(); }

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
	private:
		void RecalculateViewProjectionMatrix();
	private:
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewProjectionMatrix;

		glm::vec3 m_Position = {0.0f, 0.0f, 0.0f};
		float m_Rotation = 0.0f;
	};

}