#include "hzpch.h"

#include "Hazel/Platform/OpenGL/OpenGLTexture.h"

#include "stb_image.h"

#include "glad/glad.h"

namespace Hazel {
	
	OpenGLTexture2D::OpenGLTexture2D():
		m_Path{ "" }, m_Width{ 1 }, m_Height{ 1 }
	{
		int channels = 3;

		uint8_t data[3] = { 219, 62, 177 };

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, (channels == 3) ? GL_RGB8 : GL_RGBA8, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, GL_RGB, GL_UNSIGNED_BYTE, data);
	}

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path):
		m_Path{ path }
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = stbi_load(m_Path.c_str(), &width, &height, &channels, 0);
		HZ_CORE_ASSERT(data, "Failed to load image!");
		m_Width = width;
		m_Height = height;

		GLenum internalFormat = 0, dataFormat = 0;
		
		switch (channels) {
			case 1:
				internalFormat = GL_R8;
				dataFormat = GL_RED;
				break;
			case 3:
				internalFormat = GL_RGB8;
				dataFormat = GL_RGB;
				break;
			case 4:
				internalFormat = GL_RGBA8;
				dataFormat = GL_RGBA;
				break;
		}

		HZ_CORE_ASSERT(internalFormat & dataFormat, "Format not supported!");

		glCreateTextures(GL_TEXTURE_2D, 1, &m_RendererID);
		glTextureStorage2D(m_RendererID, 1, internalFormat, m_Width, m_Height);

		glTextureParameteri(m_RendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTextureSubImage2D(m_RendererID, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);

		stbi_image_free(data);
	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		glDeleteTextures(1, &m_RendererID);
	}

	void OpenGLTexture2D::Bind(uint32_t slot)
	{
		glBindTextureUnit(slot, m_RendererID);
	}
}