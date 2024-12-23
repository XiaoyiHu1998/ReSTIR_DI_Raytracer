#pragma once

#include "Include.h"

namespace Utils
{
	void FillFrameBufferPixel(uint32_t x, uint32_t y, glm::vec4 color, uint32_t width, FrameBufferRef frameBuffer)
	{
		int pixelIndex = (x + y * width) * 4;

		frameBuffer.get()[0][pixelIndex + 0] = std::max((unsigned char)0, std::min((unsigned char)255, static_cast<unsigned char>(color.x * 255.0f)));
		frameBuffer.get()[0][pixelIndex + 1] = std::max((unsigned char)0, std::min((unsigned char)255, static_cast<unsigned char>(color.y * 255.0f)));
		frameBuffer.get()[0][pixelIndex + 2] = std::max((unsigned char)0, std::min((unsigned char)255, static_cast<unsigned char>(color.z * 255.0f)));
		frameBuffer.get()[0][pixelIndex + 3] = std::max((unsigned char)0, std::min((unsigned char)255, static_cast<unsigned char>(color.w * 255.0f)));
	}
}