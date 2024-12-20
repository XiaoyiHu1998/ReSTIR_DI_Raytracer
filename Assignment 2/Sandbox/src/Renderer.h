#pragma once

#include "Include.h"

class Renderer
{
private:
	uint32_t m_Width, m_Height;
	std::shared_ptr<std::vector<uint8_t>> m_FrameBuffer;

private:
	void UpdateFrameBuffer(uint32_t width, uint32_t height);
public:
	Renderer(uint32_t width, uint32_t height, const std::shared_ptr<std::vector<uint8_t>>& frameBuffer):
		m_Width{width}, m_Height{height}, m_FrameBuffer{ frameBuffer }
	{}

	~Renderer() = default;

	void RenderFrame(uint32_t width, uint32_t height);
};