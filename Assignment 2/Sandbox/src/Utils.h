#pragma once

#include "Include.h"

namespace Utils
{
	void FillFrameBufferPixel(uint32_t x, uint32_t y, glm::i8vec3 color, uint32_t width, FrameBufferRef frameBuffer)
	{
		int pixelIndex = (x + y * width) * 3;
		frameBuffer.get()[0][pixelIndex + 0] = color.x;
		frameBuffer.get()[0][pixelIndex + 1] = color.y;
		frameBuffer.get()[0][pixelIndex + 2] = color.z;
	}
}

struct ColorRGB
{
	unsigned char r, g, b;

	ColorRGB(unsigned char r, unsigned char g, unsigned char b) :
		r{ r }, g{ g }, b{ b }
	{}
};