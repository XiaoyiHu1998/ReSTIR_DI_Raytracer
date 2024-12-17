#include "hzpch.h"

#include "OpenGLShader.h"

#include <fstream>

namespace Hazel {

	static GLenum ShaderTypeFromString(const std::string& type)
	{
		if (type == "vertex")
			return GL_VERTEX_SHADER;
		if (type == "fragment" || type == "pixel")
			return GL_FRAGMENT_SHADER;
		
		HZ_CORE_ASSERT(false, "Unknown shader type!");
		return 0;
	}

	OpenGLShader::OpenGLShader(const std::string& filepath)
	{
		std::string source = ReadFile(filepath);
		auto shaderSources = PreProcess(source);
		Compile(shaderSources);
	}


	OpenGLShader::OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		std::unordered_map<GLenum, std::string> sources;
		sources[GL_VERTEX_SHADER] = vertexSrc;
		sources[GL_FRAGMENT_SHADER] = fragmentSrc;
		Compile(sources);
	}

	std::string OpenGLShader::ReadFile(const std::string& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in, std::ios::binary);

		if (in)
		{
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&result[0], result.size());
			in.close();
		}
		else
		{
			HZ_CORE_ERROR("Could not open file '{0}'", filepath);
		}

		return result;
	}

	std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& source)
	{
		std::unordered_map<GLenum, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos);
			HZ_CORE_ASSERT(eol != std::string::npos, "Syntax error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			HZ_CORE_ASSERT(ShaderTypeFromString(type), "Invalid shader type specified");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)] = source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
		}

		return shaderSources;
	}

	void OpenGLShader::Compile(const std::unordered_map<GLenum, std::string>& shaderSources)
	{
		GLuint program = glCreateProgram();
		std::vector<GLenum> glShaderIDs(shaderSources.size());
		for (auto& kv : shaderSources)
		{
			GLenum type = kv.first;
			const std::string& source = kv.second;

			GLuint shader = glCreateShader(type);

			const GLchar* sourceCstr = source.c_str();
			glShaderSource(shader, 1, &sourceCstr, 0);

			glCompileShader(shader);

			GLint isCompiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE)
			{
				GLint maxLength = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

				std::vector<GLchar> infoLog(maxLength);
				glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

				glDeleteShader(shader);

				HZ_CORE_ERROR("{0}", infoLog.data())
				HZ_CORE_ASSERT(false, "Shader compilation failure!");
				break;
			}

			glAttachShader(program, shader);
			glShaderIDs.push_back(shader);
		}

		m_RendererID = program;

		// Link our program
		glLinkProgram(program);

		// Note the different functions here: glGetProgram* instead of glGetShader*.
		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, (int*)&isLinked);
		if (isLinked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);

			// We don't need the program anymore.
			glDeleteProgram(program);
			// Don't leak shaders either.

			for (auto id : glShaderIDs)
				glDeleteShader(id);

			HZ_CORE_ERROR("{0}", infoLog.data())
			HZ_CORE_ASSERT(false, "Shader link failure!");
			return;
		}

		// Always detach shaders after a successful link.
		for (auto id : glShaderIDs)
			glDetachShader(program, id);
	}

	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_RendererID);
	}

	void OpenGLShader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void OpenGLShader::UnBind() const
	{
		glUseProgram(0);
	}

	void OpenGLShader::UploadUniformInt(const std::string& uniformName, int uniformValue)
	{
		GLint uniformLocation = glGetUniformLocation(m_RendererID, uniformName.c_str());
		HZ_CORE_ASSERT(uniformLocation >= 0, "UniformLocation not found!");
		glProgramUniform1i(m_RendererID, uniformLocation, uniformValue);
	}

	void OpenGLShader::UploadUniformInt2(const std::string& uniformName, glm::i32vec2 uniformValue)
	{
		GLint uniformLocation = glGetUniformLocation(m_RendererID, uniformName.c_str());
		HZ_CORE_ASSERT(uniformLocation >= 0, "UniformLocation not found!");
		glProgramUniform2i(m_RendererID, uniformLocation, uniformValue[0], uniformValue[1]);
	}

	void OpenGLShader::UploadUniformInt3(const std::string& uniformName, glm::i32vec3 uniformValue)
	{
		GLint uniformLocation = glGetUniformLocation(m_RendererID, uniformName.c_str());
		HZ_CORE_ASSERT(uniformLocation >= 0, "UniformLocation not found!");
		glProgramUniform3i(m_RendererID, uniformLocation, uniformValue[0], uniformValue[1], uniformValue[2]);
	}

	void OpenGLShader::UploadUniformInt4(const std::string& uniformName, glm::i32vec4 uniformValue)
	{
		GLint uniformLocation = glGetUniformLocation(m_RendererID, uniformName.c_str());
		HZ_CORE_ASSERT(uniformLocation >= 0, "UniformLocation not found!");
		glProgramUniform4i(m_RendererID, uniformLocation, uniformValue[0], uniformValue[1], uniformValue[2], uniformValue[3]);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& uniformName, float uniformValue)
	{
		GLint uniformLocation = glGetUniformLocation(m_RendererID, uniformName.c_str());
		HZ_CORE_ASSERT(uniformLocation >= 0, "UniformLocation not found!");
		glProgramUniform1f(m_RendererID, uniformLocation, uniformValue);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& uniformName, const glm::vec2& uniformValue)
	{
		GLint uniformLocation = glGetUniformLocation(m_RendererID, uniformName.c_str());
		HZ_CORE_ASSERT(uniformLocation >= 0, "UniformLocation not found!");
		glProgramUniform2f(m_RendererID, uniformLocation, uniformValue[0], uniformValue[1]);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& uniformName, const glm::vec3& uniformValue)
	{
		GLint uniformLocation = glGetUniformLocation(m_RendererID, uniformName.c_str());
		HZ_CORE_ASSERT(uniformLocation >= 0, "UniformLocation not found!");
		glProgramUniform3f(m_RendererID, uniformLocation, uniformValue[0], uniformValue[1], uniformValue[2]);
	}

	void OpenGLShader::UploadUniformFloat4(const std::string& uniformName, const glm::vec4& uniformValue)
	{
		GLint uniformLocation = glGetUniformLocation(m_RendererID, uniformName.c_str());
		HZ_CORE_ASSERT(uniformLocation >= 0, "UniformLocation not found!");
		glProgramUniform4f(m_RendererID, uniformLocation, uniformValue[0], uniformValue[1], uniformValue[2], uniformValue[4]);
	}
	
	void OpenGLShader::UploadUniformMat3(const std::string& uniformName, const glm::mat3& uniformValue)
	{
		GLint uniformLocation = glGetUniformLocation(m_RendererID, uniformName.c_str());
		HZ_CORE_ASSERT(uniformLocation >= 0, "UniformLocation not found!");
		glProgramUniformMatrix3fv(m_RendererID, uniformLocation, 1, GL_FALSE, glm::value_ptr(uniformValue));
	}

	void OpenGLShader::UploadUniformMat4(const std::string& uniformName, const glm::mat4& uniformValue)
	{
		GLint uniformLocation = glGetUniformLocation(m_RendererID, uniformName.c_str());
		HZ_CORE_ASSERT(uniformLocation >= 0, "UniformLocation not found!");
		glProgramUniformMatrix4fv(m_RendererID, uniformLocation, 1, GL_FALSE, glm::value_ptr(uniformValue));
	}

}