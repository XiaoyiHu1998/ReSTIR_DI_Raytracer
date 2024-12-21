#include <stdint.h>
#include <vector>
#include <memory>

#include "glm/glm.hpp"
#include "glm/ext.hpp"


using FrameBufferRef = std::shared_ptr<std::vector<uint8_t>>;