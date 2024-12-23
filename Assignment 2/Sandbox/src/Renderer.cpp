#include "Renderer.h"

glm::vec4 Renderer::RenderRay(Ray& ray, const Triangle& triangle, const Sphere& sphere)
{
	float outputColor = 0.0f;

	outputColor = triangle.Intersect(ray) ? 1.0f : outputColor;
	outputColor = sphere.Intersect(ray) ? 0.5f : outputColor;

	return glm::vec4(outputColor);
}
