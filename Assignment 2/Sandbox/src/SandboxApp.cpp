#include <Hazel.h>

#include "imgui/imgui.h"
#include "glm/ext.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "Glad/include/glad/glad.h"

class PathTracingLayer : public Hazel::Layer
{
public:
	PathTracingLayer() :
		Layer("PathTracing")
	{
		width = 640;
		height = 480;
		channelCount = 3;
		renderTextureData.resize(width * height * 3);

		for (uint8_t& subPixel : renderTextureData)
		{
			subPixel = 255;
		}

		glGenTextures(1, &m_RenderTextureID);
		glBindTexture(GL_TEXTURE_2D, m_RenderTextureID);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, renderTextureData.data());
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void OnUpdate(Hazel::Timestep timestep) override
	{
		for (uint8_t& subPixel : renderTextureData)
		{
			subPixel = (subPixel + 1) % 255;
		}

		glBindTexture(GL_TEXTURE_2D, m_RenderTextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, renderTextureData.data());
	}

	virtual void OnImGuiRender()
	{
		ImGui::Begin("RenderTexture");
		ImGui::Image((void*)(intptr_t)m_RenderTextureID, ImVec2(640, 480));
		ImGui::End();

		//ImGui::Render();
		glViewport(0, 0, 640, 480);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void OnEvent(Hazel::Event& event) override
	{

	}

	void HandleInput(Hazel::Timestep ts) 
	{
		
	}
private:
	std::vector<uint8_t> renderTextureData;
	uint32_t m_RenderTextureID;
	uint32_t width, height;
	uint32_t channelCount = 3;
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