#include "Renderer.h"
#include "Utils.h"
#include "Settings.h"
#include "TaskBatch.h"

glm::vec3 Renderer::Reflect(const glm::vec3& incomingDirection, const glm::vec3& normal) {
	return incomingDirection - normal * 2.f * (glm::dot(incomingDirection, normal));
}

glm::vec3 Renderer::Refract(const glm::vec3& incomingDirection, const glm::vec3& normal, const float eta_t, const float eta_i) { // Snell's law
	float cosi = -std::max(-1.f, std::min(1.f, glm::dot(incomingDirection, normal)));
	if (cosi < 0) return Refract(incomingDirection, -normal, eta_i, eta_t); // if the ray comes from the inside the object, swap the air and the media
	float eta = eta_i / eta_t;
	float k = 1 - eta * eta * (1 - cosi * cosi);
	return k < 0 ? glm::vec3(1, 0, 0) : incomingDirection * eta + normal * (eta * cosi - sqrtf(k)); // k<0 = total reflection, no ray to refract. I refract it anyways, this has no physical meaning
}

glm::vec4 Renderer::RenderRay(Ray& ray, const TLAS& tlas, const Sphere& sphere)
{
	float outputColor = 0.0f;
	tlas.Traverse(ray);

	outputColor = ray.hitInfo.hit ? 1.0f : outputColor;
	outputColor = sphere.Intersect(ray) ? 0.5f : outputColor;

	return glm::vec4(outputColor);
}

void Renderer::RenderFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, const TLAS& tlas, const Sphere& sphere)
{
	TaskBatch taskBatch(Settings::ThreadCount);

	for (uint32_t y = 0; y < height; y += Settings::RenderingKernelSize)
		for (uint32_t x = 0; x < width; x += Settings::RenderingKernelSize)
		{
			taskBatch.EnqueueTask([=]() { RenderKernelFrameBuffer(camera, frameBuffer, width, height, x, y, tlas, sphere); });
		}

	taskBatch.ExecuteTasks();
}

void Renderer::RenderKernelFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const Sphere& sphere)
{
	uint32_t xMax = std::min(xMin + Settings::RenderingKernelSize, width);
	uint32_t yMax = std::min(yMin + Settings::RenderingKernelSize, height);

	for (uint32_t y = yMin; y < yMax; y++)
		for (uint32_t x = xMin; x < xMax; x++)
		{
			Ray ray = camera.GetRay(x, y);
			glm::vec4 color = Renderer::RenderRay(ray, tlas, sphere);

			//float xColor = float(x) / static_cast<float>(m_CurrentWidth);
			//glm::vec4 color = glm::vec4(xColor, yColor, 0.0f, 1.0f);

			Utils::FillFrameBufferPixel(x, y, color, width, frameBuffer);
		}
}
