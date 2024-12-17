#include "hzpch.h"
#include "Hazel/Renderer/Material.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Platform/OpenGL/OpenGLMaterial.h"

namespace Hazel {

	Material* Material::Create(Ref<Shader> shader)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:	HZ_CORE_ASSERT(false, "RendererAPI::API::None not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:	return new OpenGLMaterial(shader);
		}

		HZ_CORE_ASSERT(false, "RendererAPI::API::None not supported!");
		return nullptr;
	}

}