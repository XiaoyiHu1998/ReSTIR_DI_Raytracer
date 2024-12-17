#include "hzpch.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RenderCommand.h"
#include "Hazel/Renderer/Material.h"

#include "Hazel/Platform/OpenGL/OpenGLShader.h"

namespace Hazel {
	Renderer::SceneData* Renderer::m_SceneData = new SceneData;

	void Renderer::Init()
	{
		RenderCommand::Init();
	}

	void Renderer::BeginScene(Ref<PerspectiveCamera> camera)
	{
		Renderer::m_SceneData->ViewProjectionMatrix = camera->GetViewProjectionMatrix();
	}

	void Renderer::BeginScene(Ref<OrthographicCamera> camera)
	{
		Renderer::m_SceneData->ViewProjectionMatrix = camera->GetViewProjectionMatrix();
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::Submit(const Ref<Material>& material, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
	{
		material->Bind();
		std::dynamic_pointer_cast<OpenGLShader>(material->GetShader())->UploadUniformMat4("u_ViewProjection", Renderer::m_SceneData->ViewProjectionMatrix);
		std::dynamic_pointer_cast<OpenGLShader>(material->GetShader())->UploadUniformMat4("u_Transform", transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}

}