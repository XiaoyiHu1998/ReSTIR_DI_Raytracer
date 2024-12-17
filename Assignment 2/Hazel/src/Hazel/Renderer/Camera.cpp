#include "hzpch.h"
#include "Hazel/Renderer/Camera.h"

#include "glm/ext.hpp"

namespace Hazel {

	PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float clipDistanceNear, float clipDistanceFar,
										PerspectiveCamera::ControlMode controlMode):
		fov{fov}, aspectRatio{aspectRatio}, clipDistanceNear{clipDistanceNear}, clipDistanceFar{clipDistanceFar}, controlMode{controlMode}
	{
		m_ViewMatrix = glm::mat4(1);
		m_ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, clipDistanceNear, clipDistanceFar);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

		transform = Transform();
		targetLocation = glm::vec3(0.0f, 0.0f, 0.0f);

		m_RightDirection = glm::vec3(1.0f, 0.0f, 0.0f);
		m_UpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
		m_BackDirection = glm::vec3(0.0f, 0.0f, 1.0f);
		m_TargetLocation = transform.translation - m_BackDirection;
	}

	void PerspectiveCamera::RecalculateDirections()
	{
		if (controlMode == PerspectiveCamera::ControlMode::TargetLock)
		{
			m_TargetLocation = targetLocation;
		}
		else
		{
			glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.x), glm::vec3(1, 0, 0));
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(transform.rotation.y), glm::vec3(0, 1, 0));
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));

			m_TargetLocation = glm::vec4(transform.translation, 1.0f) + (glm::vec4(0.0f, 0.0f, -1.0f, 1.0f) * rotationMatrix);
		}

		m_BackDirection = glm::normalize(transform.translation - m_TargetLocation);
		m_RightDirection = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), m_BackDirection));
		m_UpDirection = glm::normalize(glm::cross(m_BackDirection, m_RightDirection));
	}

	void PerspectiveCamera::RecalculateViewMatrix()
	{
		RecalculateDirections();
		m_ViewMatrix = glm::lookAt(transform.translation, m_TargetLocation, m_UpDirection);
	}

	void PerspectiveCamera::RecalculateProjectionMatrix()
	{
		m_ProjectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, clipDistanceNear, clipDistanceFar);
	}

	void PerspectiveCamera::RecalculateViewProjectionMatrix()
	{
		RecalculateViewMatrix();
		RecalculateProjectionMatrix();
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
	
	OrthographicCamera::OrthographicCamera(float left, float right, float top, float bottom) :
		m_ProjectionMatrix{ glm::ortho(left, right, top, bottom, -1.0f, 1.0f) }, m_ViewMatrix{ glm::mat4(1.0f) }
	{
		m_Position = glm::vec3(0.0f);
		m_Rotation = 0.0f;

		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::RecalculateViewProjectionMatrix()
	{
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position);
		m_ViewMatrix = glm::rotate(m_ViewMatrix, glm::radians(m_Rotation), glm::vec3(0.0f, 0.0f, 1.0f));

		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

}