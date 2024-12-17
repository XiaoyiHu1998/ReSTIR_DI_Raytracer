#include "hzpch.h"
#include "Application.h"

#include "Hazel/Input.h"
#include "Hazel/Renderer/Renderer.h"

#include "GLFW/glfw3.h"

namespace Hazel
{
#define BIND_EVENT_FN(x) std::bind(&Application::x,	this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application() 
	{
		HZ_CORE_ASSERT(!s_Instance, "Application instance already exists!");
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		Renderer::Init();

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}


	Application::~Application() {

	}

	void Application::PushLayer(Layer* layer)
	{
		m_layerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_layerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

		for (auto it = m_layerStack.end(); it != m_layerStack.begin();)
		{
			(*--it)->OnEvent(e);
			if ( e.IsHandled() )
				break;
		}
	}

	void Application::Run() {
		float time = glfwGetTime();
		Timestep ts = time - m_LastFrameTime;
		m_LastFrameTime = time;
		
		while (m_Running)
		{
			for (Layer* layer : m_layerStack)
				layer->OnUpdate(ts);

			m_ImGuiLayer->Begin();
			for (Layer* layer : m_layerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
}