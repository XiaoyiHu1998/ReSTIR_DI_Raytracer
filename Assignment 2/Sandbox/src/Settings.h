#pragma once

#include <thread>

#include "Include.h"

namespace Settings
{
	// Add settings to be used globally in program and ImGui
	uint32_t ThreadCount = std::thread::hardware_concurrency() - 2;

	uint32_t RenderResolutionWidth = 640;
	uint32_t RenderResolutionHeight = 480;
	uint32_t RenderingKernelSize = 16;
	uint32_t SamplePerPixel = 1;

	uint32_t MaxRayDepth = 8;

}