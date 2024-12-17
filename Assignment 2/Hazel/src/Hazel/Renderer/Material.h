#pragma once

#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/Texture.h"

namespace Hazel {

	class Material
	{
	public:
		virtual ~Material() {}

		virtual void SetShader(Ref<Shader> shader) = 0;
		virtual Ref<Shader> GetShader() = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual void SetColor(const glm::vec3& color) = 0;
		virtual void SetTexture2D(Ref<Texture2D> texture) = 0;

		static Material* Create(Ref<Shader> shader);
	};

}