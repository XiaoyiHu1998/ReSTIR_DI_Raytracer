#include "Renderer.h"

void Renderer::UpdateFrameBuffer(uint32_t width, uint32_t height)
{
	m_Width = width;
	m_Height = height;
	uint32_t frameBufferSize = width * height * 3;

	if (frameBufferSize != m_FrameBuffer->size())
		m_FrameBuffer->resize(frameBufferSize, 0);
	else
		m_FrameBuffer->clear();
}


void Renderer::RenderFrame(uint32_t width, uint32_t height)
{
	UpdateFrameBuffer(width, height);
}
