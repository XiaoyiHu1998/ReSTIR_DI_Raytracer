#include <Hazel.h>

#include "imgui/imgui.h"
#include "glm/ext.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "Glad/include/glad/glad.h"

#include "Include.h"
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
		m_CurrentWidth = m_NextWidth = 1280;
		m_CurrentHeight = m_NextHeight = 720;

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

		RenderCommand::InitFrameBuffer(m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);

		brightness++;

		for (uint8_t& subPixel : *m_FrameBuffer.get())
		{
			subPixel = brightness % 255;
		}

		RenderCommand::SetFrameBufferTexture(m_FrameBufferID, m_FrameBuffer, m_CurrentWidth, m_CurrentHeight);
	}

	virtual void OnImGuiRender()
	{
		ImGui::Begin("RenderTexture");

		ImVec2 nextFrameResolution = ImGui::GetContentRegionAvail();
		m_NextWidth = nextFrameResolution.x;
		m_NextHeight = nextFrameResolution.y;

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

	// Temp
	uint8_t brightness = 0;
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