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

		for (uint8_t& subPixel : *m_FrameBuffer.get())
		{
			subPixel = 255;
		}

		RenderCommand::GenerateFrameBufferTexture(m_FrameBufferID, m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);
	}

	void OnUpdate(Hazel::Timestep timestep) override
	{
		m_CurrentWidth = m_NextWidth;
		m_CurrentHeight = m_NextHeight;
		m_Camera.SetResolution(m_CurrentWidth, m_CurrentHeight);
		colorBuffer.resize(m_CurrentWidth * m_CurrentHeight, ColorRGB(0,0,0));

		m_FrameBuffer->resize(m_CurrentWidth * m_CurrentHeight * 3, 0);

		RenderCommand::InitFrameBuffer(m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);

		//brightness++;
		//brightness = 0;
		//
		//for (uint8_t& subPixel : *m_FrameBuffer.get())
		//{
		//	subPixel = brightness;
		//	brightness++;
		//}

		Triangle triangle = Triangle(
			glm::vec3(0,-10,0),
			glm::vec3(10,10,0),
			glm::vec3(-10,10,0)
		);

		Sphere sphere = Sphere(glm::vec3(0,0,0), 1.0f);

		for (int y = 0; y < m_CurrentHeight; y++)
		{
			for (int x = 0; x < m_CurrentWidth; x++)
			{
				//Ray ray = m_Camera.GetRay(x, y);
				//glm::u8vec3 color = Renderer::RenderRay(ray, triangle, sphere);
				
				uint8_t xColor = static_cast<float>(x) / static_cast<float>(m_CurrentWidth) * 255.0f;
				uint8_t yColor = static_cast<float>(y) / static_cast<float>(m_CurrentHeight) * 255.0f;
				glm::u8vec3 color = glm::u8vec3(xColor, yColor, 0.0f);

				colorBuffer[x + y * m_CurrentWidth] = ColorRGB(xColor, yColor, 0);
		 
				//Utils::FillFrameBufferPixel(x, y, color, m_CurrentWidth, m_FrameBuffer);
				//Utils::FillFrameBufferPixel(x, y, color, m_CurrentWidth, m_FrameBuffer);
				//Utils::FillFrameBufferPixel(x, y, color, m_CurrentWidth, m_FrameBuffer);

				//if (x == m_CurrentWidth - 2 && y == m_CurrentHeight - 1)
				//{
				//	std::cout << static_cast<float>(xColor) << std::endl;
				//	std::cout << static_cast<uint32_t>(m_FrameBuffer.get()[0][(x + y * m_CurrentWidth) * 3]) << "," << static_cast<uint32_t>(m_FrameBuffer.get()[0][(x + y * m_CurrentWidth) * 3 + 1]) << "," << static_cast<uint32_t>(m_FrameBuffer.get()[0][(x + y * m_CurrentWidth) * 3 + 2]) << std::endl;
				//}
			}
		}

		for (int i = 0; i < colorBuffer.size(); i++)
		{
			m_FrameBuffer.get()[0][i * 3 + 0] = colorBuffer[i].r;
			m_FrameBuffer.get()[0][i * 3 + 1] = colorBuffer[i].g;
			m_FrameBuffer.get()[0][i * 3 + 2] = colorBuffer[i].b;
		}


		RenderCommand::SetFrameBufferTexture(m_FrameBufferID, m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);
	}

	virtual void OnImGuiRender()
	{
		DrawImGuiDockSpace();
		ImGui::Begin("Viewport");

		ImVec2 nextFrameResolution = ImGui::GetContentRegionAvail();
		m_NextWidth = nextFrameResolution.x;
		m_NextHeight = nextFrameResolution.y;

		ImGui::Image((void*)(intptr_t)m_FrameBufferID, ImVec2(m_NextWidth, m_NextHeight));
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
	std::vector<ColorRGB> colorBuffer;
	uint32_t m_FrameBufferID;
	uint32_t m_CurrentWidth, m_CurrentHeight;
	uint32_t m_NextWidth, m_NextHeight;

	// World state
	Camera m_Camera;
	//Acceleration structure (objects + emmisives)
	//Acceleration structure (emmisives only)

	// Temp
	uint8_t brightness = 0;
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