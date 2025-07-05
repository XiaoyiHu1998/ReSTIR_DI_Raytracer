#pragma once

#include "Hazel/Events/Event.h"


namespace Hazel {

	class HAZEL_API WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(int width, int height) :
			m_Width{ width }, m_Height{ height }
		{}

		inline int GetWidth() const { return m_Width; }
		inline int GetHeight() const { return m_Height; }

		std::string ToString() const override {
			std::stringstream ss;
			ss << "WindowResizeEvent: " << GetWidth() << ", " << GetHeight();
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		int m_Width, m_Height;
	};

	class HAZEL_API WindowMovedEvent : public Event
	{
	public:
		WindowMovedEvent(int positionX, int positionY) :
			m_PositionX{ positionX }, m_PositionY{ positionY }
		{}

		inline int GetPositionX() const { return m_PositionX; }
		inline int GetPositionY() const { return m_PositionY; }

		std::string ToString() const override {
			std::stringstream ss;
			ss << "WindowMovedEvent: " << GetPositionX() << ", " << GetPositionY();
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowMoved)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		int m_PositionX, m_PositionY;
	};

	class HAZEL_API WindowFocusEvent : public Event
	{
	public:
		WindowFocusEvent() {}

		EVENT_CLASS_TYPE(WindowFocus)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HAZEL_API WindowLostFocusEvent : public Event
	{
	public:
		WindowLostFocusEvent() {}

		EVENT_CLASS_TYPE(WindowLostFocus)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HAZEL_API WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() {}

		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HAZEL_API AppTickEvent : public Event
	{
	public:
		AppTickEvent() {}

		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HAZEL_API AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent() {}

		EVENT_CLASS_TYPE(AppUpdate)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class HAZEL_API AppRenderEvent : public Event
	{
	public:
		AppRenderEvent() {}

		EVENT_CLASS_TYPE(AppRender)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

}