#include <stdint.h>
#include <vector>
#include <memory>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <glm/gtx/string_cast.hpp>

using FrameBufferRef = std::shared_ptr<std::vector<unsigned char>>;