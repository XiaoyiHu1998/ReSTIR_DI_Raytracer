# pragma once

#include "tiny_obj_loader.h"

#include "Include.h"
#include "Primitives.h"
#include "Transform.h"

namespace GeometryLoader
{
	bool LoadObj(const std::string& filepath, std::vector<Triangle>& triangles);
}
