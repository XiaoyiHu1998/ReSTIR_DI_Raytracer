#pragma once

#include "Include.h"

class PathTracer
{
private:
	uint32_t m_Width, m_Height;
	std::shared_ptr<std::vector<uint8_t>> m_RenderTextureData;
public:
	PathTracer(uint32_t width, uint32_t height, const std::shared_ptr<std::vector<uint8_t>>& renderTextureData):
		m_Width{width}, m_Height{height}, m_RenderTextureData{renderTextureData}
	{}

	~PathTracer() = default;

	void RenderFrame();
};