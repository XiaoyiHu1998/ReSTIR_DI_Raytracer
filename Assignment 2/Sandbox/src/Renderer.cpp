#include "Renderer.h"

glm::u8vec3 Renderer::RenderRay(Ray& ray, const Triangle& triangle, const Sphere& sphere)
{
	uint8_t outputColor = 0;

	outputColor = triangle.Intersect(ray) ? 255 : outputColor;
	outputColor = sphere.Intersect(ray) ? 125 : outputColor;

	return glm::u8vec3(outputColor);
}
