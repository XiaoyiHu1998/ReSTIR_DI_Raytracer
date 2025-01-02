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

    uint32_t PCGHash(uint32_t& seed)
    {
        uint32_t state = seed * 747796405u + 2891336453u;
        uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        return (word >> 22u) ^ word;
    }

    uint32_t WangHash(uint32_t& seed)
    {
        seed = (seed ^ 61) ^ (seed >> 16);
        seed *= 9;
        seed = seed ^ (seed >> 4);
        seed *= 0x27d4eb2d;
        seed = seed ^ (seed >> 15);
        return seed;
    }

    float RandomFloat(uint32_t& seed)
    {
        seed = PCGHash(seed);
        return static_cast<float>(seed) / static_cast<float>(std::numeric_limits<uint32_t>().max());
    }

    int RandomInt(int minInclusive, int maxExclusive, uint32_t& seed)
    {
        float randomFloat = RandomFloat(seed) * static_cast<float>(maxExclusive);
        int randomInt = static_cast<int>(randomFloat);
        randomInt = randomInt == maxExclusive ? maxExclusive - 1 : randomInt; // In case of issues

        return randomInt;
    }
}