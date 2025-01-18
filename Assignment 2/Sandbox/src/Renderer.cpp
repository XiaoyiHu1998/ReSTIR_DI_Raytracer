#include "Renderer.h"

#include <chrono>
#include <functional>

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


LightSampleInfo Renderer::SampleRandomLight(const glm::vec3& hitLocation, const TLAS& tlasEmmisive, uint32_t& seed)
{
	LightSampleInfo lightInfo;

	uint32_t randomLightIndex = Utils::RandomInt(0, tlasEmmisive.GetObjectCount(), seed);
	std::shared_ptr<BLAS> lightBLAS = tlasEmmisive.GetBLAS(randomLightIndex);
	Triangle randomTriangle = lightBLAS->GetRandomTriangle(lightInfo.Probability, seed);
	glm::vec3 randomPoint = randomTriangle.GetRandomPoint(seed);

	lightInfo.location = randomPoint;
	lightInfo.direction = randomPoint - hitLocation;
	lightInfo.distance = glm::length(lightInfo.direction);
	lightInfo.direction = glm::normalize(lightInfo.direction);
	lightInfo.Area = lightBLAS->GetArea();
	lightInfo.normal = randomTriangle.GetNormal();
	lightInfo.emissiveIntensity = lightBLAS->GetMaterial().EmissiveIntensity;
	lightInfo.emissiveColor = lightBLAS->GetMaterial().EmissiveColor;
	lightInfo.Probability = (1.0f / static_cast<float>(tlasEmmisive.GetObjectCount())) * (1.0f / lightBLAS->GetArea());

	return lightInfo;
}


LightSampleInfo Renderer::SampleRandomLight(const glm::vec3& hitLocation, const std::vector<Sphere>& sphereLights, uint32_t& seed)
{
	LightSampleInfo lightInfo;

	uint32_t randomLightIndex = Utils::RandomInt(0, sphereLights.size(), seed);
	Sphere sphere = sphereLights[randomLightIndex];

	lightInfo.location = sphere.position;
	lightInfo.direction = sphere.position - hitLocation;
	lightInfo.distance = glm::length(lightInfo.direction);
	lightInfo.direction = glm::normalize(lightInfo.direction);
	lightInfo.Area = sphere.Area();
	lightInfo.normal = -1.0f * lightInfo.direction;
	lightInfo.emissiveIntensity = sphere.material.EmissiveIntensity;
	lightInfo.emissiveColor = sphere.material.EmissiveColor;
	lightInfo.Probability = 1.0f / static_cast<float>(sphereLights.size());

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

namespace RenderModeNormal
{
	glm::vec4 RenderRay(Ray& ray, const TLAS& tlas, uint32_t& seed)
	{
		glm::vec3 E(0.1f);

		tlas.Traverse(ray);
		if (ray.hitInfo.hit)
			E = 0.5 * ray.hitInfo.normal + glm::vec3(0.5f);

		return glm::vec4(E, 1.0f);
	}
}

namespace RenderModeTraversalSteps
{
	glm::vec4 RenderRay(Ray& ray, const TLAS& tlas, uint32_t& seed)
	{
		glm::vec3 E(0.1f);

		tlas.Traverse(ray);
		if (ray.hitInfo.hit)
			E = 0.5 * glm::vec3(ray.hitInfo.traversalStepsHitBVH / 100.0f) + glm::vec3(0.5f);

		return glm::vec4(E, 1.0f);
	}
}


glm::vec4 Renderer::RenderDI(Ray& ray, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed)
{
	glm::vec3 E(0);
	
	// Intersection Test
	tlas.Traverse(ray);
	if (!ray.hitInfo.hit)
		return glm::vec4(0.8f, 0.2f, 0.8f, 1.0f);

	auto CalcLightContribution = [=](const Ray& ray, const Sphere& sphere) {
		//Sphere sphere = sphereLights[Utils::RandomInt(0, sphereLights.size(), seed)]; 
		glm::vec3 lightColor = sphere.material.EmissiveColor;
		float lightIntensity = sphere.material.EmissiveIntensity;
		glm::vec3 lightDirection = sphere.position - ray.hitInfo.location;
		float lightDistance = glm::length(lightDirection);
		lightDirection = glm::normalize(lightDirection);

		if (glm::dot(ray.hitInfo.normal, lightDirection) > 0)
		{
			Ray shadowRay = Ray(ray.hitInfo.location + (m_Settings.Eta * lightDirection), lightDirection, lightDistance - 2 * m_Settings.Eta);
			bool lightOccluded = tlas.IsOccluded(shadowRay);
			if (!lightOccluded || !m_Settings.LightOcclusionCheckDI)
			{
				glm::vec3 BRDF = ray.hitInfo.material.Albedo / M_PI;
				return lightIntensity * lightColor * glm::dot(ray.hitInfo.normal, lightDirection) / (lightDistance * lightDistance);
			}
		}

		return glm::vec3(0);
	};

	if (m_Settings.SampleAllLightsDI)
	{
		for (int i = 0; i < sphereLights.size(); i++)
		{
			E += CalcLightContribution(ray, sphereLights[i]);
		}
	}
	else {
		for (int i = 0; i < m_Settings.CandidateCountDI; i++)
		{
			int index = Utils::RandomInt(0, sphereLights.size(), seed);
			Sphere randomSphere = sphereLights[index];
			E += CalcLightContribution(ray, randomSphere);
		}
	}

	//glm::vec3 distanceColor = glm::vec3(ray.hitInfo.distance / 1000);
	return glm::vec4(E, 1.0f);
}

void Renderer::RenderFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, const TLAS& tlas, const TLAS& tlasEmmisive, const std::vector<Sphere>& sphereLights)
{
	auto timeStart = std::chrono::system_clock::now();
	TaskBatch taskBatch(m_Settings.ThreadCount);

	for (uint32_t y = 0; y < height; y += m_Settings.RenderingKernelSize)
	{
		for (uint32_t x = 0; x < width; x += m_Settings.RenderingKernelSize)
		{
			uint32_t seed = x + y * width;
			taskBatch.EnqueueTask([=]() { RenderKernelFrameBuffer(camera, frameBuffer, width, height, x, y, tlas, tlasEmmisive, sphereLights, seed); });
		}
	}

	taskBatch.ExecuteTasks();
	auto timeEnd = std::chrono::system_clock::now();
	m_LastFrameTime = std::chrono::duration<float, std::ratio<1,1000>>(timeEnd - timeStart).count();
}

void Renderer::RenderKernelFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const TLAS& tlasEmmisive, const std::vector<Sphere>& sphereLights, uint32_t seed)
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
				uint32_t index = x + y * width;
				switch (m_Settings.Mode)
				{
					case Settings::RenderMode::Normals:
						colorAccumulator += RenderModeNormal::RenderRay(ray, tlas, seed);
						break;
					case Settings::RenderMode::TraversalSteps:
						colorAccumulator += RenderModeTraversalSteps::RenderRay(ray, tlas, seed);
						break;
					case Settings::RenderMode::DI:
						colorAccumulator += RenderDI(ray, tlas, sphereLights, seed);
						break;
					case Settings::RenderMode::ReSTIR:
						GenerateSample(camera, glm::i32vec2(x, y), index, tlas, sphereLights, seed);
						colorAccumulator += RenderSample(m_SampleBuffer[index], tlas, seed);
						break;
				}
			}

			colorAccumulator /= static_cast<float>(m_Settings.SamplesPerPixel);
			Utils::FillFrameBufferPixel(x, y, colorAccumulator, width, frameBuffer);
		}
	}
}

Sample Renderer::SampleAreaLights(const Camera& camera, const glm::i32vec2& pixel, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed)
{
	Ray ray = camera.GetRay(pixel.x, pixel.y);
	tlas.Traverse(ray); // Edge case: hitting emmisive on first vertex

	int lightIndex = Utils::RandomInt(0, sphereLights.size(), seed);
	Sphere randomSphere = sphereLights[lightIndex];
	glm::vec3 lightDirection = randomSphere.position - ray.hitInfo.location;
	float lightDistance = glm::length(lightDirection);
	lightDirection = glm::normalize(randomSphere.position - ray.hitInfo.location);

	bool validDirectLighting = true;
	Ray shadowRay;
	if (m_Settings.LightOcclusionCheckCandidatesReSTIR)
	{
		if (glm::dot(ray.hitInfo.normal, lightDirection) > 0)
		{
			shadowRay = Ray(ray.hitInfo.location + m_Settings.Eta * lightDirection, lightDirection, lightDistance - 2 * m_Settings.Eta);
			tlas.Traverse(shadowRay);
		}

		validDirectLighting = !shadowRay.hitInfo.hit;
	}

	// Construct Sample
	Sample sample;
	sample.valid = ray.hitInfo.hit && ray.hitInfo.material.MaterialType != Material::Type::Emissive && validDirectLighting;
	sample.Path.CameraOrigin = camera.GetTransform().translation;
	sample.Path.HitLocation = ray.hitInfo.location;
	sample.Path.LightLocation = randomSphere.position;
	sample.Path.FirstRayHitInfo = ray.hitInfo;
	sample.Path.ShadowRayHitInfo = shadowRay.hitInfo;
	sample.Path.Light = randomSphere;
	sample.Weight = 1.0f / sphereLights.size(); // 1 / PDF

	return sample;
}

void Renderer::GenerateSample(const Camera& camera, const glm::i32vec2 pixel, uint32_t bufferIndex, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed)
{
	Resevoir<Sample> resevoir;
	Sample sample;

	auto colorToContribution = [](const glm::vec3& color) {
		return glm::length(color);
	};

	for (int i = 0; i < m_Settings.CandidateCountReSTIR; i++)
	{
		sample = SampleAreaLights(camera, pixel, tlas, sphereLights, seed);
		float weight = (1.0f / static_cast<float>(m_Settings.CandidateCountReSTIR)) * colorToContribution(TargetDistribution(sample.Path)) * sample.Weight; // Should mutiply with p-hat Sample.x
		resevoir.Update(sample, weight, seed);
	}

	sample = resevoir.GetSample();
	float weight = 1 / colorToContribution(TargetDistribution(sample.Path)) * resevoir.GetWeightTotal();
	m_SampleBuffer[bufferIndex] = Sample(sample, 1.0f / m_Settings.CandidateCountReSTIR);
}

glm::vec3 Renderer::TargetDistribution(const PathDI& path)
{
	glm::vec3 lightDirection = path.LightLocation - path.HitLocation;
	float lightDistance = glm::length(lightDirection);
	lightDirection = glm::normalize(lightDirection);

	return path.Light.material.EmissiveIntensity * path.Light.material.EmissiveColor * glm::dot(path.FirstRayHitInfo.normal, lightDirection) / (lightDistance * lightDistance);
}

glm::vec4 Renderer::RenderSample(Sample sample, const TLAS& tlas, uint32_t& seed)
{
	if (!sample.valid)
	{
		// Handle Hitting Emmisive Surface on first hit
		Material material = sample.Path.FirstRayHitInfo.material;
		bool hitEmmisiveSurface = sample.Path.FirstRayHitInfo.hit && material.MaterialType == Material::Type::Emissive;
		glm::vec3 outputColor = hitEmmisiveSurface ? material.EmissiveColor : glm::vec3(0.0f);

		return glm::vec4(outputColor, 1.0f);
	}

	// Direct lighting calculation
	glm::vec3 outputColor(0.0f);

	glm::vec3 lightDirection = sample.Path.LightLocation - sample.Path.HitLocation;
	float lightDistance = glm::length(lightDirection);
	lightDirection = glm::normalize(lightDirection);
	
	bool validDirectLighting = true;
	if (m_Settings.LightOcclusionCheckShadingReSTIR)
	{
		Ray shadowRay;
		if (glm::dot(sample.Path.FirstRayHitInfo.normal, lightDirection) > 0)
		{
			shadowRay = Ray(sample.Path.FirstRayHitInfo.location + m_Settings.Eta * lightDirection, lightDirection, lightDistance - 2 * m_Settings.Eta);
			tlas.Traverse(shadowRay);
		}

		validDirectLighting = !shadowRay.hitInfo.hit;
	}

	if (validDirectLighting)
		outputColor = TargetDistribution(sample.Path);

	return glm::vec4(outputColor, 1.0f) ;
}
