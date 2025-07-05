#include "hzpch.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Platform/OpenGL/OpenGLRendererAPI.h"

namespace Hazel {

	RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;

}