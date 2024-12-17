#include "hzpch.h"
#include "Hazel/Platform/OpenGL/OpenGLMaterial.h"

#include "glad/glad.h"
#include "OpenGLShader.h"

namespace Hazel {

	OpenGLMaterial::OpenGLMaterial(Ref<Shader> shader):
		m_Shader{shader},
		m_Texture2D{ Texture2D::Create() },
		u_Color{ glm::vec3(0) }
	{}

	void OpenGLMaterial::SetShader(Ref<Shader> shader)
	{
		m_Shader = shader;
	}

	Ref<Shader> OpenGLMaterial::GetShader()
	{
		return m_Shader;
	}

	void OpenGLMaterial::Bind()
	{
		m_Shader->Bind();
		std::dynamic_pointer_cast<OpenGLShader>(m_Shader)->UploadUniformFloat3("u_Color", u_Color);

		m_Texture2D->Bind(0);
		std::dynamic_pointer_cast<OpenGLShader>(m_Shader)->UploadUniformInt("u_Texture", 0);
	}

	void OpenGLMaterial::UnBind()
	{
		m_Shader->UnBind();
	}

}