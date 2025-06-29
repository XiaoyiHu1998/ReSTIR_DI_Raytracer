#include "RenderCommand.h"

#include <iostream>

void RenderCommand::GeneratePixelBufferObject(uint32_t& pixelObjectBufferID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height)
{
	glGenBuffers(1, &pixelObjectBufferID);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelObjectBufferID);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4 * sizeof(unsigned char), frameBuffer->data(), GL_DYNAMIC_DRAW);
}

void RenderCommand::DeletePixelBufferObject(uint32_t& pixelBufferObjectID)
{
	glDeleteBuffers(1, &pixelBufferObjectID);
}

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

void RenderCommand::InitFrame(uint32_t& frameBufferID, uint32_t& pixelBufferObjectID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height)
{
	// Bind PBO
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBufferObjectID); //TODO replace with a raw TGexImage2D upload in case of issues

	// Update Texture
	glBindTexture(GL_TEXTURE_2D, frameBufferID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
}

void RenderCommand::UploadFrameData(uint32_t& frameBufferID, uint32_t& pixelBufferObjectID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height)
{
	// Update PBO
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBufferObjectID);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, frameBuffer->size() * sizeof(frameBuffer.get()[0][0]), frameBuffer->data(), GL_STATIC_DRAW);

	// Update Texture
	glBindTexture(GL_TEXTURE_2D, frameBufferID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Unbind PBO to prevent interference with ImGui rendering
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}
void RenderCommand::Clear()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}
