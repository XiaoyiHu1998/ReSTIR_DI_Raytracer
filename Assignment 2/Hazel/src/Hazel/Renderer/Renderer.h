#pragma once

#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/Camera.h"
#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/Material.h"

namespace Hazel {

	class Renderer {
	public:
		static void Init();

		static void BeginScene(Ref<PerspectiveCamera> camera);
		static void BeginScene(Ref<OrthographicCamera> camera);
		static void EndScene();

		static void Submit(const Ref<Material>& material, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};
	
		static SceneData* m_SceneData;
	};

}