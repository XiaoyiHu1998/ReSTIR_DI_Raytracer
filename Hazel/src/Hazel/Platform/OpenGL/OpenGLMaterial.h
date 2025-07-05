#pragma once

#include "Hazel/Renderer/Material.h"
#include "glm/glm.hpp"

namespace Hazel {

	class OpenGLMaterial : public Material
	{
	public:
		OpenGLMaterial(Ref<Shader> shader);
		~OpenGLMaterial() {}

		virtual void SetShader(Ref<Shader> shader) override;
		virtual Ref<Shader> GetShader() override;

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual void SetColor(const glm::vec3& color) override { u_Color = color; };
		virtual void SetTexture2D(Ref<Texture2D> texture) override { m_Texture2D = texture; };
	public:
		Ref<Shader> m_Shader;
		Ref<Texture2D> m_Texture2D;
		glm::vec3 u_Color;
	};

}