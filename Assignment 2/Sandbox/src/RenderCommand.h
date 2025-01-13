# pragma once

#include "Glad/include/glad/glad.h"

#include "Include.h"

class RenderCommand
{
public:
	static void GeneratePixelBufferObject(uint32_t& pixelObjectBufferID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height);
	static void DeletePixelBufferObject(uint32_t& pixelBufferObjectID);
	static void GenerateFrameBufferTexture(uint32_t& frameBufferID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height);
	static void DeleteFrameBufferTexture(uint32_t& frameBufferID);
	static void RegenerateFrameBufferTexture(uint32_t& frameBufferID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height);
	static void InitFrame(uint32_t& frameBufferID, uint32_t& pixelBufferObjectID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height);
	static void UploadFrameData(uint32_t& frameBufferID, uint32_t& pixelBufferObjectID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height);
	static void Clear();
};