# pragma once

#include "Glad/include/glad/glad.h"

#include "Include.h"

class RenderCommand
{
public:
	static void GenerateFrameBufferTexture(uint32_t& frameBufferID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height);
	static void DeleteFrameBufferTexture(uint32_t& frameBufferID);
	static void RegenerateFrameBufferTexture(uint32_t& frameBufferID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height);
	static void SetFrameBufferTexture(uint32_t& frameBufferID, FrameBufferRef frameBuffer, uint32_t width, uint32_t height);
	static void InitFrameBuffer(FrameBufferRef frameBuffer, uint32_t width, uint32_t height);
	static void Clear();
};