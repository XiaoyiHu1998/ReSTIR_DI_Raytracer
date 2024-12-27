#include "Renderer.h"

glm::vec4 Renderer::RenderRay(Ray& ray, const TLAS& tlas, const Sphere& sphere)
{
	float outputColor = 0.0f;
	tlas.Traverse(ray);

	outputColor = ray.hitInfo.hit ? 1.0f : outputColor;
	outputColor = sphere.Intersect(ray) ? 0.5f : outputColor;

	return glm::vec4(outputColor);
}
