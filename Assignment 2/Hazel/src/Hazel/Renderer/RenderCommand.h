#pragma once

#include "glm/glm.hpp"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	class RenderCommand
	{
	public:
		inline static void Init()
		{
			s_RendererAPI->Init();
		}

		inline static void DrawIndexed(const Ref<VertexArray>& vertexArray)
		{
			s_RendererAPI->DrawIndexed(vertexArray);
		}

		inline static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		inline static void Clear()
		{
			s_RendererAPI->Clear();
		}
	private:
		static RendererAPI* s_RendererAPI;
	};

}