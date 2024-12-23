#include "RenderCommand.h"

#include <iostream>

void RenderCommand::GenerateFrameBufferTexture(uint32_t& frameBufferID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height)
{
	glGenTextures(1, &frameBufferID);
	glBindTexture(GL_TEXTURE_2D, frameBufferID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frameBuffer->data());
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void RenderCommand::DeleteFrameBufferTexture(uint32_t& frameBufferID)
{
	glDeleteTextures(1, &frameBufferID);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderCommand::RegenerateFrameBufferTexture(uint32_t& frameBufferID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height)
{
	frameBuffer->resize(width * height * 4, 0);

	DeleteFrameBufferTexture(frameBufferID);
	GenerateFrameBufferTexture(frameBufferID, frameBuffer, width, height);
}

void RenderCommand::SetFrameBufferTexture(uint32_t& frameBufferID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height)
{
	glBindTexture(GL_TEXTURE_2D, frameBufferID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frameBuffer->data());
	glGenerateMipmap(GL_TEXTURE_2D);
}

void RenderCommand::InitFrameBuffer(FrameBufferRef frameBuffer, uint32_t width, uint32_t height)
{
	if (width * height * 4 != frameBuffer->size())
	{
		frameBuffer->resize(width * height * 4, 0);
	}
}

void RenderCommand::Clear()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}