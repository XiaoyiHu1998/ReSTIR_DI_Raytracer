#ifndef UTILS_HPP
#define UTILS_HPP

#include <stdint.h>
#include <cmath>
#include <limits>
#include <vector>
#include <algorithm>

namespace Utils
{
    uint32_t PCGHash(uint32_t& seed);
    

    uint32_t WangHash(uint32_t & seed);
   

    float RandomFloat(uint32_t & seed);
    

    int RandomInt(int minInclusive, int maxInclusive, uint32_t& seed);
   
}

#endif //UTILS_HPP