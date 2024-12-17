#pragma once

#include "Core.h"

#include "Window.h"
#include "Hazel/LayerStack.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/ApplicationEvent.h"

#include "Hazel/ImGui/ImGuiLayer.h"

#include "Hazel/Renderer/Shader.h"

#include "Hazel/Renderer/Buffer.h"
#include "Hazel/Renderer/VertexArray.h"

namespace Hazel {

	class HAZEL_API Application {
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		inline Window& GetWindow() { return *m_Window; }
		
		inline static Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);
	private:
		float m_LastFrameTime = 0.0f;
		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		LayerStack m_layerStack;
	private:
		static Application* s_Instance;
	};

	//To be defined in client
	Application* CreateApplication();
}

