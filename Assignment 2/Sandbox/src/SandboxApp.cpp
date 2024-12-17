#include <Hazel.h>

#include "imgui/imgui.h"
#include "glm/ext.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

class ExampleLayer : public Hazel::Layer
{
public:
	ExampleLayer() :
		Layer("Example")
	{
		// Camera
		orthographicCamera = std::make_shared<Hazel::OrthographicCamera>(-1.6, 1.6, -0.9, 0.9);
		perspectiveCamera = std::make_shared<Hazel::PerspectiveCamera>(55, 16.0f / 9.0f, 0.001f, 1000.0f);
		perspectiveCamera->transform.translation += glm::vec3(0.0f, 0.0f, 5.0f);

		// Triangle
		m_VertexArray.reset(Hazel::VertexArray::Create());

		float vertices[3 * 7] = {
			-0.5f, -0.5f, 0.0f, 0.9f, 0.0f, 0.9f, 1.0f,
			0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.9f, 1.0f,
			0.0f, 0.5f, 0.0f, 0.9f, 0.9f, 0.0f, 1.0f
		};

		Hazel::Ref<Hazel::VertexBuffer> vertexBuffer;
		vertexBuffer.reset(Hazel::VertexBuffer::Create(vertices, sizeof(vertices)));
		Hazel::BufferLayout layout = {
					{Hazel::ShaderDataType::Float3, "a_Position"},
					{Hazel::ShaderDataType::Float4, "a_Color"},
		};
		vertexBuffer->SetLayout(layout);
		m_VertexArray->AddVertexBuffer(vertexBuffer);

		unsigned int indices[3] = { 0, 1, 2 };
		Hazel::Ref<Hazel::IndexBuffer> indexBuffer;
		indexBuffer.reset(Hazel::IndexBuffer::Create(indices, sizeof(indices) / sizeof(unsigned int)));
		m_VertexArray->SetIndexBuffer(indexBuffer);

		// Square
		m_SquareVA.reset(Hazel::VertexArray::Create());

		float squareVertices[3 * 4] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.5f,  0.5f, 0.0f,
			-0.5f,  0.5f, 0.0f
		};

		Hazel::Ref<Hazel::VertexBuffer> squareVB;
		squareVB.reset(Hazel::VertexBuffer::Create(squareVertices, sizeof(squareVertices)));
		squareVB->SetLayout({
			{ Hazel::ShaderDataType::Float3, "a_Position" }
		});
		m_SquareVA->AddVertexBuffer(squareVB);

		uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };
		Hazel::Ref<Hazel::IndexBuffer> squareIB;
		squareIB.reset(Hazel::IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t)));
		m_SquareVA->SetIndexBuffer(squareIB);


		// Textured Quad
		m_TextureVA.reset(Hazel::VertexArray::Create());

		float textureVertices[5 * 4] = {
			-0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
			 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			-0.5f,  0.5f, 0.0f, 0.0f, 1.0f
		};

		Hazel::Ref<Hazel::VertexBuffer> textureVB;
		textureVB.reset(Hazel::VertexBuffer::Create(textureVertices, sizeof(textureVertices)));
		textureVB->SetLayout({
			{ Hazel::ShaderDataType::Float3, "a_Position" },
			{ Hazel::ShaderDataType::Float2, "a_TexCoord" }
			});
		m_TextureVA->AddVertexBuffer(textureVB);

		uint32_t textureIndices[6] = { 0, 1, 2, 2, 3, 0 };
		Hazel::Ref<Hazel::IndexBuffer> textureIB;
		textureIB.reset(Hazel::IndexBuffer::Create(textureIndices, sizeof(textureIndices) / sizeof(uint32_t)));
		m_TextureVA->SetIndexBuffer(textureIB);

		std::string vertexSrc = R"(
			#version 330 core

			layout(location = 0) in vec3 a_Position;
			layout(location = 1) in vec4 a_Color;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;
			uniform vec3 u_Color;

			out vec3 v_Position;
			out vec4 v_Color;

			void main()
			{
				v_Position = a_Position;
				v_Color = a_Color;
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0f);
			}

		)";

		std::string fragmentSrc = R"(
			#version 330 core

			layout(location = 0) out vec4 color;
			uniform vec3 u_Color;

			in vec3 v_Position;
			in vec4 v_Color;

			void main()
			{
				color = vec4(v_Position * 0.5 + 0.5, 1.0);
				color = v_Color;
			}

		)";

		m_Shader.reset(Hazel::Shader::Create(vertexSrc, fragmentSrc));
		m_Material.reset(Hazel::Material::Create(m_Shader));

		std::string flatColorShaderVertexSrc = R"(
			#version 330 core
			
			layout(location = 0) in vec3 a_Position;

			uniform mat4 u_ViewProjection;
			uniform mat4 u_Transform;
			uniform vec3 u_Color;

			out vec3 v_Position;

			void main()
			{
				v_Position = a_Position;
				gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0f);
			}
		)";

		std::string flatColorShaderFragmentSrc = R"(
			#version 330 core
			
			layout(location = 0) out vec4 color;
			uniform vec3 u_Color;

			in vec3 v_Position;

			void main()
			{
				color = vec4(0.2, 0.3, 0.8, 1.0);
				color = vec4(u_Color, 1.0);
			}
		)";

		m_FlatColorShader.reset(Hazel::Shader::Create(flatColorShaderVertexSrc, flatColorShaderFragmentSrc));
		m_FlatColorMaterial.reset(Hazel::Material::Create(m_FlatColorShader));
		m_FlatColorMaterial->SetColor(glm::vec3(0.2f, 0.3f, 0.8f));

		m_TextureShader.reset(Hazel::Shader::Create("assets/shaders/Texture.glsl"));
		m_Texture = Hazel::Texture2D::Create("assets/textures/Checkerboard.png");
		m_Texture->Bind(0);

		m_TextureMaterial.reset(Hazel::Material::Create(m_TextureShader));
		m_TextureMaterial->SetTexture2D(m_Texture);

		m_TextureChernoLogo = Hazel::Texture2D::Create("assets/textures/ChernoLogo.png");
		m_TextureChernoLogo->Bind(0);

		m_TextureChernoLogoMaterial.reset(Hazel::Material::Create(m_TextureShader));
		m_TextureChernoLogoMaterial->SetTexture2D(m_TextureChernoLogo);
	}

	void OnUpdate(Hazel::Timestep timestep) override
	{
		HandleInput(timestep);

		Hazel::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
		Hazel::RenderCommand::Clear();

		static glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
		
		m_FlatColorMaterial->SetColor(m_SquareColor);

		Hazel::Renderer::BeginScene(perspectiveCamera);
		{
			for (int y = 0; y < 20; y++)
			{
				for (int x = 0; x < 20; x++)
				{
					glm::vec3 pos = glm::vec3(x * 0.11 - 1.05f, y * 0.11 - 1.05f, 0.0f);
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) * scale;
					Hazel::Renderer::Submit(m_FlatColorMaterial, m_SquareVA, transform);
				}
			}

			glm::mat4 triangleTransform = glm::translate(glm::scale(glm::mat4(1), glm::vec3(0.3f, 0.3f, 0.3f)), glm::vec3(0.0f, 2.5f, 0.0f));
			Hazel::Renderer::Submit(m_Material, m_VertexArray, triangleTransform);

			Hazel::Renderer::Submit(m_TextureMaterial, m_TextureVA);
			Hazel::Renderer::Submit(m_TextureChernoLogoMaterial, m_TextureVA);
		}
		Hazel::Renderer::EndScene();
	}

	virtual void OnImGuiRender()
	{
		ImGui::Begin("Settings");
		ImGui::ColorEdit3("Square Color", glm::value_ptr(m_SquareColor));
		ImGui::End();
	}

	void OnEvent(Hazel::Event& event) override
	{

	}

	void HandleInput(Hazel::Timestep ts) {
		float moveSpeed = 0.25f * ts;
		float turnSpeed = 3.0f * ts;

		if (Hazel::Input::IsKeyPressed(HZ_KEY_W))
		{
			if (Hazel::Input::IsKeyPressed(HZ_KEY_LEFT_ALT))
			{
				perspectiveCamera->transform.rotation -= perspectiveCamera->GetRightDirection() * turnSpeed;
			}
			else
			{
				perspectiveCamera->transform.translation += perspectiveCamera->GetForwardDirection() * moveSpeed;
			}
		}
		else if (Hazel::Input::IsKeyPressed(HZ_KEY_S))
		{
			if (Hazel::Input::IsKeyPressed(HZ_KEY_LEFT_ALT))
			{
				perspectiveCamera->transform.rotation += perspectiveCamera->GetRightDirection() * turnSpeed;
			}
			else
			{
				perspectiveCamera->transform.translation -= perspectiveCamera->GetForwardDirection() * moveSpeed;
			}
		}

		if (Hazel::Input::IsKeyPressed(HZ_KEY_A))
		{
			if (Hazel::Input::IsKeyPressed(HZ_KEY_LEFT_ALT))
			{
				perspectiveCamera->transform.rotation -= perspectiveCamera->GetUpDirection() * turnSpeed;
			}
			else
			{
				perspectiveCamera->transform.translation -= perspectiveCamera->GetRightDirection() * moveSpeed;
			}
		}
		else if (Hazel::Input::IsKeyPressed(HZ_KEY_D))
		{
			if (Hazel::Input::IsKeyPressed(HZ_KEY_LEFT_ALT))
			{
				perspectiveCamera->transform.rotation += perspectiveCamera->GetUpDirection() * turnSpeed;
			}
			else
			{
				perspectiveCamera->transform.translation += perspectiveCamera->GetRightDirection() * moveSpeed;
			}
		}

		if (Hazel::Input::IsKeyPressed(HZ_KEY_Q))
		{
			perspectiveCamera->transform.translation -= perspectiveCamera->GetUpDirection() * moveSpeed;
		}
		else if (Hazel::Input::IsKeyPressed(HZ_KEY_E))
		{
			perspectiveCamera->transform.translation += perspectiveCamera->GetUpDirection() * moveSpeed;
		}

		if (Hazel::Input::IsKeyPressed(HZ_KEY_R))
		{
			perspectiveCamera->transform.translation = glm::vec3(0.0f, 0.0f, 5.0f);
			perspectiveCamera->transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		}

		if (Hazel::Input::IsKeyPressed(HZ_KEY_C))
		{
			perspectiveCamera->controlMode = Hazel::PerspectiveCamera::ControlMode::FirstPerson;
		}
		else if (Hazel::Input::IsKeyPressed(HZ_KEY_X))
		{
			perspectiveCamera->controlMode = Hazel::PerspectiveCamera::ControlMode::TargetLock;
		}
	}
private:
	Hazel::Ref<Hazel::Material> m_Material;
	Hazel::Ref<Hazel::Material> m_FlatColorMaterial;
	Hazel::Ref<Hazel::Material> m_TextureMaterial;
	Hazel::Ref<Hazel::Material> m_TextureChernoLogoMaterial;

	Hazel::Ref<Hazel::Shader> m_Shader;
	Hazel::Ref<Hazel::Shader> m_FlatColorShader;
	Hazel::Ref<Hazel::Shader> m_TextureShader;

	Hazel::Ref<Hazel::VertexArray> m_VertexArray;
	Hazel::Ref<Hazel::VertexArray> m_SquareVA;
	Hazel::Ref<Hazel::VertexArray> m_TextureVA;

	Hazel::Ref<Hazel::Texture2D> m_Texture;
	Hazel::Ref<Hazel::Texture2D> m_TextureChernoLogo;

	Hazel::Ref<Hazel::PerspectiveCamera> perspectiveCamera;
	Hazel::Ref<Hazel::OrthographicCamera> orthographicCamera;

	glm::vec3 m_SquareColor = { 0.2f, 0.3f, 0.8f };

};

class Sandbox : public Hazel::Application
{
public:
	Sandbox() 
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox() 
	{

	}
};

Hazel::Application* Hazel::CreateApplication() {
	return new Sandbox();
}