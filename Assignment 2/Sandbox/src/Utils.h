#pragma once

#include "Include.h"

namespace Utils
{
    static inline float Clamp(float value, float max, float min)
    {
        return std::min(max, std::max(min, value));
    }

	static inline void FillFrameBufferPixel(uint32_t x, uint32_t y, glm::vec4 color, uint32_t width, FrameBufferRef frameBuffer)
	{
		int pixelIndex = (x + y * width) * 4;

		frameBuffer.get()[0][pixelIndex + 0] = static_cast<unsigned char>(Clamp(color.x * 255.0f, 255.0f, 0.0f));
		frameBuffer.get()[0][pixelIndex + 1] = static_cast<unsigned char>(Clamp(color.y * 255.0f, 255.0f, 0.0f));
		frameBuffer.get()[0][pixelIndex + 2] = static_cast<unsigned char>(Clamp(color.z * 255.0f, 255.0f, 0.0f));
		frameBuffer.get()[0][pixelIndex + 3] = static_cast<unsigned char>(Clamp(color.w * 255.0f, 255.0f, 0.0f));
	}

    static inline uint32_t PCGHash(uint32_t& seed)
    {
        uint32_t state = seed * 747796405u + 2891336453u;
        uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        return (word >> 22u) ^ word;
    }

    static inline uint32_t WangHash(uint32_t& seed)
    {
        seed = (seed ^ 61) ^ (seed >> 16);
        seed *= 9;
        seed = seed ^ (seed >> 4);
        seed *= 0x27d4eb2d;
        seed = seed ^ (seed >> 15);
        return seed;
    }

    static inline float RandomFloat(uint32_t& seed)
    {
        seed = PCGHash(seed);
        return static_cast<float>(seed) / static_cast<float>(std::numeric_limits<uint32_t>().max());
    }

    static inline int RandomInt(int minInclusive, int maxExclusive, uint32_t& seed)
    {
        float randomFloat = RandomFloat(seed) * static_cast<float>(maxExclusive);
        int randomInt = static_cast<int>(randomFloat);
        randomInt = randomInt == maxExclusive ? maxExclusive - 1 : randomInt; // In case of issues

        return randomInt;
    }

    static inline float colorToContribution(const glm::vec3& color) 
    {
        // TODO: Replace with max channel value in rgb channels
        return glm::length(color);
    };

    static inline glm::i32vec2 GetNeighbourPixel(glm::i32vec2 pixel, glm::i32vec2 resolution, float radius, uint32_t& seed)
    {
        glm::i32vec2 neighbourPixel = glm::i32vec2(-1, -1);

        // Check if neighour is within the frame and max radius
        while (0 > neighbourPixel.x || neighbourPixel.x >= resolution.x || 0 > neighbourPixel.y || neighbourPixel.y >= resolution.y || (neighbourPixel - pixel).length() > radius)
        {
            neighbourPixel.x = pixel.x + Utils::RandomInt(0, radius, seed);
            neighbourPixel.y = pixel.y + Utils::RandomInt(0, radius, seed);
        }

        return neighbourPixel;
    }

}