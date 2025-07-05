#pragma once

#include "Hazel/Events/Event.h"


namespace Hazel {

	class HAZEL_API MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(float x, float y):
			m_MouseX{x}, m_MouseY{y}
		{}

		inline float GetX() const { return m_MouseX; }
		inline float GetY() const { return m_MouseY; }


		std::string ToString() const override {
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
	
	private:
		float m_MouseX, m_MouseY;
	};

	class HAZEL_API MouseScrolledEvent : public Event 
	{
	public:
		MouseScrolledEvent(float offsetX, float offsetY):
			m_OffsetX{offsetX}, m_OffsetY{offsetY}
		{}

		inline float GetOffsetX() const { return m_OffsetX; }
		inline float GetOffsetY() const { return m_OffsetY; }

		std::string ToString() const override {
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << GetOffsetX() << ", " << GetOffsetY();
			return ss.str();
		}
		
		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
	
	private:
		float m_OffsetX, m_OffsetY;
	};


	class HAZEL_API MouseButtonEvent : public Event
	{
	public:
		inline int GetMouseButton() { return m_MouseButton; }

		EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse | EventCategoryMouseButton)

	protected:
		MouseButtonEvent(int mouseButton): 
			m_MouseButton{mouseButton}
		{}

		int m_MouseButton;
	};


	class HAZEL_API MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(int mouseButton, int repeatCount) :
			MouseButtonEvent(mouseButton)//, m_RepeatCount{ repeatCount }
		{}

		//inline int GetRepeatCount() { return m_RepeatCount; }

		std::string ToString() const override {
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_MouseButton /* << " (" << m_RepeatCount << " repeats)"*/;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)

	private:
		//int m_RepeatCount;
	};


	class HAZEL_API MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(int mouseButton) :
			MouseButtonEvent(mouseButton)
		{}

		std::string ToString() const override {
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_MouseButton;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}