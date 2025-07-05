#pragma once 

#include "Hazel/Renderer/Shader.h"
#include "glad/glad.h"

namespace Hazel {

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& filepath);
        OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        virtual ~OpenGLShader();

        virtual void Bind() const;
        virtual void UnBind() const;

        void UploadUniformInt(const std::string& uniformName, int uniformValue);
        void UploadUniformInt2(const std::string& uniformName, glm::i32vec2 uniformValue);
        void UploadUniformInt3(const std::string& uniformName, glm::i32vec3 uniformValue);
        void UploadUniformInt4(const std::string& uniformName, glm::i32vec4 uniformValue);

        void UploadUniformFloat(const std::string& uniformName, float uniformValue);
        void UploadUniformFloat2(const std::string& uniformName, const glm::vec2& uniformValue);
        void UploadUniformFloat3(const std::string& uniformName, const glm::vec3& uniformValue);
        void UploadUniformFloat4(const std::string& uniformName, const glm::vec4& uniformValue);

        void UploadUniformMat3(const std::string& uniformName, const glm::mat3& uniformValue);
        void UploadUniformMat4(const std::string& uniformName, const glm::mat4& uniformValue);
    private:
        std::string ReadFile(const std::string& filepath);
        std::unordered_map<GLenum, std::string> PreProcess(const std::string& shaderSources);
        void Compile(const std::unordered_map<GLenum, std::string>& shaderSources);
    private:
        uint32_t m_RendererID;
    };

}