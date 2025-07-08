# pragma once

#include "tiny_obj_loader.h"
#include "tiny_bvh.h"

#include "Include.h"
#include "Transform.h"

namespace GeometryLoader
{
	bool LoadObj(const std::string& filepath, std::vector<tinybvh::bvhvec4>& vertices);
}
