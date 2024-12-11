#include "Utils.h"

namespace Utils
{
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
        return float(seed) / float(std::numeric_limits<uint32_t>().max());
    }

    int RandomInt(int minInclusive, int maxInclusive, uint32_t& seed)
    {
        float randomFloat = RandomFloat(seed) * (float)maxInclusive;
        int roundingValue = (randomFloat - std::floor(randomFloat)) >= 0.5f ? 1 : 0;

        return (int)randomFloat + roundingValue;
    }
}