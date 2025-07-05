#include "hzpch.h"
#include "Hazel/Renderer/RenderCommand.h"

#include "Hazel/Platform/OpenGL/OpenGLRendererAPI.h"

namespace Hazel {

	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI;

}