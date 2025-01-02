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


glm::vec3 Renderer::RandomPointOnHemisphere(const glm::vec3& normal, uint32_t& seed)
{
	glm::vec3 randomVector = glm::normalize(glm::vec3(Utils::RandomFloat(seed), Utils::RandomFloat(seed), Utils::RandomFloat(seed)));

	while (glm::dot(randomVector, normal) <= 0)
	{
		randomVector = glm::normalize(glm::vec3(Utils::RandomFloat(seed), Utils::RandomFloat(seed), Utils::RandomFloat(seed)));
	}

	return randomVector;
}


glm::vec3 Renderer::CosineSampleHemisphere(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& tangent, uint32_t& seed)
{
	float r0 = Utils::RandomFloat(seed);
	float r1 = Utils::RandomFloat(seed);
	float radius = std::sqrt(r0);
	float theta = 2 * M_PI * r1;
	float x = radius * std::cos(theta);
	float y = radius * std::sin(theta);
	glm::vec3 randomPoint(x, y, std::sqrt(1 - r0)); // point on hemisphere around +z axis
	
	// Rotate 180 degrees such that the point is on the hemisphere with -z axis before using glm::lookAt transform
	glm::mat4 transform = glm::lookAt(position, position + normal, tangent);
	randomPoint = glm::vec3(randomPoint.x, randomPoint.y, randomPoint.z * -1.0f);

	return glm::normalize(transform * glm::vec4(randomPoint, 1.0f));
}


glm::vec4 Renderer::RenderRay(Ray& ray, const TLAS& tlas, const TLAS& tlasEmmisive, uint32_t& seed)
{
	glm::vec3 T(1);
	glm::vec3 E(0);
	uint32_t currentRayDepth = 0;

	while (currentRayDepth < Settings::MaxRayDepth)
	{
		// Intersection Test
		tlas.Traverse(ray);
		if (!ray.hitInfo.hit)
			break;

		glm::vec3 BRDF = ray.hitInfo.material.Albedo / M_PI;
		if (ray.hitInfo.material.MaterialType == Material::Type::Emissive)
			break;

		// Direct lighting contribution
		//ShadowRay();

		// Indirect lighting contribution
		glm::vec3 triangleTangent = ray.hitInfo.tangent;
		//glm::vec3 reflectionDirection = RandomPointOnHemisphere(ray.hitInfo.normal, seed);
		glm::vec3 reflectionDirection = CosineSampleHemisphere(ray.hitInfo.location, ray.hitInfo.normal, triangleTangent, seed);
		//float hemispherePDF = 1.0f / (M_PI / 2.0f);
		float hemispherePDF = glm::dot(ray.hitInfo.normal, reflectionDirection) / M_PI;
		Ray nextRay = Ray(ray.hitInfo.location, reflectionDirection);
		T *= (glm::dot(ray.hitInfo.normal, reflectionDirection) / hemispherePDF) * BRDF;
		
		currentRayDepth++;
	}

	return glm::vec4(E, 1.0f);
}

void Renderer::RenderFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, const TLAS& tlas, const TLAS& tlasEmmisive)
{
	TaskBatch taskBatch(Settings::ThreadCount);

	for (uint32_t y = 0; y < height; y += Settings::RenderingKernelSize)
	{
		for (uint32_t x = 0; x < width; x += Settings::RenderingKernelSize)
		{
			uint32_t seed = x + y * width;
			taskBatch.EnqueueTask([=]() { RenderKernelFrameBuffer(camera, frameBuffer, width, height, x, y, tlas, tlasEmmisive, seed); });
		}
	}

	taskBatch.ExecuteTasks();
}

void Renderer::RenderKernelFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const TLAS& tlasEmmisive, uint32_t seed)
{
	uint32_t xMax = std::min(xMin + Settings::RenderingKernelSize, width);
	uint32_t yMax = std::min(yMin + Settings::RenderingKernelSize, height);

	for (uint32_t y = yMin; y < yMax; y++)
	{
		for (uint32_t x = xMin; x < xMax; x++)
		{
			Ray ray = camera.GetRay(x, y);
			glm::vec4 color = Renderer::RenderRay(ray, tlas, tlasEmmisive, seed);

			Utils::FillFrameBufferPixel(x, y, color, width, frameBuffer);
		}
	}
		
}
