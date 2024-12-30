#pragma once

#include "Include.h"

namespace Settings
{
	// Add settings to be used globally in program and ImGui

	// Rendering Resolution (independent from imgui viewport size)
	uint32_t RenderingKernelSize;
	uint32_t ThreadCount = 4;
	// uint32_t Samples per pixel
	// enum Render paths

}