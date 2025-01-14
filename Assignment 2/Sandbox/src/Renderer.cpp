#include "Renderer.h"

#include <chrono>

#include "Utils.h"
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


LightSampleInfo Renderer::SampleRandomLight(const Ray& ray, const TLAS& tlasEmmisive, uint32_t& seed)
{
	uint32_t randomLightId = Utils::RandomInt(0, tlasEmmisive.GetObjectCount(), seed);
	std::shared_ptr<BLAS> lightBLAS = tlasEmmisive.GetBLAS(randomLightId);
	Triangle randomTriangle = lightBLAS->GetRandomTriangle(seed);
	glm::vec3 randomPoint = randomTriangle.GetRandomPoint(seed);

	LightSampleInfo lightInfo;
	lightInfo.direction = randomPoint - ray.hitInfo.location;
	lightInfo.distance = glm::length(lightInfo.direction);
	lightInfo.direction = glm::normalize(lightInfo.direction);
	lightInfo.Area = lightBLAS->GetArea();
	lightInfo.normal = randomTriangle.GetNormal();
	lightInfo.intensity = lightBLAS->GetMaterial().Emmitance;

	return lightInfo;
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
	glm::vec3 E(0.1f);
	uint32_t currentRayDepth = 0;
	float eta = 0.01f;

	while (currentRayDepth < m_Settings.MaxRayDepth)
	{
		if (m_Settings.Mode == Settings::RenderMode::Normals)
		{
			tlas.Traverse(ray);
			if (ray.hitInfo.hit)
				E = 0.5 * ray.hitInfo.normal + glm::vec3(0.5f);

			return glm::vec4(E, 1.0f);
		}

		if (m_Settings.Mode == Settings::RenderMode::TraversalSteps)
		{
			tlas.Traverse(ray);
			if (ray.hitInfo.hit)
				E = 0.5 * glm::vec3(ray.hitInfo.traversalStepsHitBVH / 100.0f) + glm::vec3(0.5f);

			return glm::vec4(E, 1.0f);
		}

		// Intersection Test
		tlas.Traverse(ray);
		if (!ray.hitInfo.hit)
			break;

		if (ray.hitInfo.material.MaterialType == Material::Type::Emissive)
		{
			if (currentRayDepth == 0)
				E = ray.hitInfo.material.Emmitance;

			break;
		}

		glm::vec3 BRDF = ray.hitInfo.material.Albedo / M_PI;

		// Direct lighting contribution
		LightSampleInfo lightInfo = SampleRandomLight(ray, tlasEmmisive, seed);
		float normalDotLightDirection = glm::dot(ray.hitInfo.normal, lightInfo.direction);
		float lightNormalDotLightDirection = glm::dot(lightInfo.normal, -lightInfo.direction);

		if (normalDotLightDirection > 0 && lightNormalDotLightDirection > 0)
		{
			Ray shadowRay = Ray(ray.hitInfo.location + eta * lightInfo.direction, lightInfo.direction, lightInfo.distance - eta);
			tlas.Traverse(shadowRay);

			if (shadowRay.hitInfo.hit)
			{
				//std::cout << "light hit - intensity: " << lightInfo.intensity << std::endl;
				float solidAngle = (lightNormalDotLightDirection * lightInfo.Area) / (lightInfo.distance * lightInfo.distance);
				float lightPDF = 1 / solidAngle;
				E += T * (normalDotLightDirection / solidAngle) * BRDF * lightInfo.intensity;
			}
		}

		// Indirect lighting contribution
		glm::vec3 reflectionDirection = CosineSampleHemisphere(ray.hitInfo.location, ray.hitInfo.normal, ray.hitInfo.tangent, seed);
		float hemispherePDF = glm::dot(ray.hitInfo.normal, reflectionDirection) / M_PI;

		//glm::vec3 reflectionDirection = RandomPointOnHemisphere(ray.hitInfo.normal, seed);
		//float hemispherePDF = 1.0f / (M_PI / 2.0f);

		Ray nextRay = Ray(ray.hitInfo.location + eta * reflectionDirection, reflectionDirection);
		T *= (glm::dot(ray.hitInfo.normal, reflectionDirection) / hemispherePDF) * BRDF;
		
		currentRayDepth++;
	}

	return glm::vec4(E, 1.0f);
}

void Renderer::RenderFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, const TLAS& tlas, const TLAS& tlasEmmisive)
{
	auto timeStart = std::chrono::system_clock::now();
	TaskBatch taskBatch(m_Settings.ThreadCount);
	//TaskBatch taskBatch(1);

	for (uint32_t y = 0; y < height; y += m_Settings.RenderingKernelSize)
	{
		for (uint32_t x = 0; x < width; x += m_Settings.RenderingKernelSize)
		{
			uint32_t pixelIndex = x + y * width;
			taskBatch.EnqueueTask([=]() { RenderKernelFrameBuffer(camera, frameBuffer, width, height, x, y, tlas, tlasEmmisive, pixelIndex); });
			uint32_t seed = x + y * width;
			taskBatch.EnqueueTask([=]() { RenderKernelFrameBuffer(camera, frameBuffer, width, height, x, y, tlas, tlasEmmisive, seed); });
		}
	}

	taskBatch.ExecuteTasks();
	auto timeEnd = std::chrono::system_clock::now();

	m_LastFrameTime = std::chrono::duration<float, std::ratio<1,1000>>(timeEnd - timeStart).count();
}

void Renderer::RenderKernelFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const TLAS& tlasEmmisive, uint32_t seed)
{
	uint32_t xMax = std::min(xMin + m_Settings.RenderingKernelSize, width);
	uint32_t yMax = std::min(yMin + m_Settings.RenderingKernelSize, height);

	seed += static_cast<uint32_t>(std::time(nullptr));

	for (uint32_t y = yMin; y < yMax; y++)
	{
		for (uint32_t x = xMin; x < xMax; x++)
		{
			Ray ray = camera.GetRay(x, y);
			glm::vec4 colorAccumulator = glm::vec4(0);

			for (int i = 0; i < m_Settings.SamplesPerPixel; i++)
			{
				colorAccumulator += Renderer::RenderRay(ray, tlas, tlasEmmisive, seed);
			}

			colorAccumulator /= static_cast<float>(m_Settings.SamplesPerPixel);

			Utils::FillFrameBufferPixel(x, y, colorAccumulator, width, frameBuffer);
		}
	}
		
}
