#include <Hazel.h>

#include "imgui/imgui.h"
#include "glm/ext.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "Glad/include/glad/glad.h"

#include "Include.h"
#include "Utils.h"
#include "RenderCommand.h"
#include "Renderer.h"
#include "Camera.h"

class PathTracingLayer : public Hazel::Layer
{
public:
	PathTracingLayer() :
		Layer("PathTracing")
	{
		m_FrameBuffer = std::make_shared<std::vector<uint8_t>>();
		m_CurrentWidth = m_NextWidth = 640;
		m_CurrentHeight = m_NextHeight = 480;
		m_Camera = Camera(m_CurrentWidth, m_CurrentHeight, 40);

		RenderCommand::GenerateFrameBufferTexture(m_FrameBufferID, m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);
	}

	void OnUpdate(Hazel::Timestep timestep) override
	{
		m_CurrentWidth = m_NextWidth;
		m_CurrentHeight = m_NextHeight;
		m_Camera.SetResolution(m_CurrentWidth, m_CurrentHeight);
		RenderCommand::InitFrameBuffer(m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);

		Triangle triangle = Triangle(
			glm::vec3(0,-10,0),
			glm::vec3(10,10,0),
			glm::vec3(-10,10,0)
		);

		Sphere sphere = Sphere(glm::vec3(0,0,0), 1.0f);

		for (uint32_t y = 0; y < m_CurrentHeight; y++)
		{
			//float yColor = float(y) / static_cast<float>(m_CurrentHeight);

			for (uint32_t x = 0; x < m_CurrentWidth; x++)
			{
				Ray ray = m_Camera.GetRay(x, y);
				glm::vec4 color = Renderer::RenderRay(ray, triangle, sphere);
				
				//float xColor = float(x) / static_cast<float>(m_CurrentWidth);
				//glm::vec4 color = glm::vec4(xColor, yColor, 0.0f, 1.0f);

				Utils::FillFrameBufferPixel(x, y, color, m_CurrentWidth, m_FrameBuffer);
			}
		}

		RenderCommand::SetFrameBufferTexture(m_FrameBufferID, m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);
	}

	virtual void OnImGuiRender()
	{
		DrawImGuiDockSpace();
		ImGui::Begin("Viewport");

		ImVec2 nextFrameResolution = ImGui::GetContentRegionAvail();

		m_NextWidth = std::max((uint32_t)1, static_cast<uint32_t>(nextFrameResolution.x));
		m_NextHeight = std::max((uint32_t)1, static_cast<uint32_t>(nextFrameResolution.y));

		ImGui::Image((void*)(intptr_t)m_FrameBufferID, ImVec2(m_CurrentWidth, m_CurrentHeight));
		ImGui::End();

		RenderCommand::Clear();
	}

	void OnEvent(Hazel::Event& event) override
	{

	}

	void HandleInput(Hazel::Timestep ts) 
	{
		
	}
private:
	// Output Configuration
	FrameBufferRef m_FrameBuffer;
	uint32_t m_FrameBufferID;
	uint32_t m_CurrentWidth, m_CurrentHeight;
	uint32_t m_NextWidth, m_NextHeight;

	// World state
	Camera m_Camera;
	//Acceleration structure (objects + emmisives)
	//Acceleration structure (emmisives only)
private:
	void DrawImGuiDockSpace()
	{
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		static bool dockspaceOpen = true;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		ImGuiViewport* viewport_ImGui = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport_ImGui->WorkPos);
		ImGui::SetNextWindowSize(viewport_ImGui->WorkSize);
		ImGui::SetNextWindowViewport(viewport_ImGui->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
			window_flags |= ImGuiWindowFlags_NoBackground;
		}

		// Important: keep maindockspace always open
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();
		ImGui::PopStyleVar(2);

		// Main dockspace, this is where all our windows are drawn
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("import obj")) {
					//importFile = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		ImGui::End();
	}
};

class Sandbox : public Hazel::Application
{
public:
	Sandbox() 
	{
		PushLayer(new PathTracingLayer());
	}

	~Sandbox() 
	{

	}
};

Hazel::Application* Hazel::CreateApplication() {
	return new Sandbox();
}