#include "Renderer.h"

#include <chrono>
#include <functional>

#include "Utils.h"
#include "TaskBatch.h"
#include "RenderCommand.h"

namespace RenderModeNormal
{
	glm::vec4 RenderRay(Ray& ray, const TLAS& tlas, uint32_t& seed)
	{
		glm::vec3 E(0.1f);

		tlas.Traverse(ray);
		if (ray.hitInfo.hit)
			E = 0.5f * ray.hitInfo.normal + glm::vec3(0.5f);

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
			E = 0.5f * glm::vec3(ray.hitInfo.traversalStepsHitBVH / 100.0f) + glm::vec3(0.5f);

		return glm::vec4(E, 1.0f);
	}
}


glm::vec4 Renderer::RenderDI(Ray& ray, const TLAS& tlas, const std::vector<PointLight>& pointLights, uint32_t& seed)
{
	glm::vec3 E(0.0f);
	
	// Intersection Test
	tlas.Traverse(ray);
	if (!ray.hitInfo.hit)
		return glm::vec4(0.8f, 0.2f, 0.8f, 1.0f);

	auto CalcLightContribution = [=](const Ray& ray, const PointLight& pointLight) {
		glm::vec3 lightDirection = pointLight.position - ray.hitInfo.location;
		float lightDistance = glm::length(lightDirection);
		lightDirection = glm::normalize(lightDirection);

		if (glm::dot(ray.hitInfo.normal, lightDirection) > 0)
		{
			Ray shadowRay = Ray(ray.hitInfo.location + (m_Settings.Eta * lightDirection), lightDirection, lightDistance - 2 * m_Settings.Eta);
			bool lightOccluded = tlas.IsOccluded(shadowRay);
			if (!lightOccluded || !m_Settings.LightOcclusionCheckDI)
			{
				//glm::vec3 BRDF = ray.hitInfo.material.Albedo / M_PI;
				float BRDF = glm::dot(ray.hitInfo.normal, lightDirection);
				return BRDF * pointLight.material.EmissiveIntensity * pointLight.material.EmissiveColor / (lightDistance * lightDistance);
			}
		}

		return glm::vec3(0);
	};

	if (m_Settings.SampleAllLightsDI)
	{
		for (int i = 0; i < pointLights.size(); i++)
		{
			E += CalcLightContribution(ray, pointLights[i]);
		}
	}
	else {
		for (int i = 0; i < m_Settings.CandidateCountDI; i++)
		{
			int index = Utils::RandomInt(0, pointLights.size(), seed);
			PointLight randomPointLight = pointLights[index];
			E += CalcLightContribution(ray, randomPointLight);
		}
	}

	return glm::vec4(E, 1.0f);
}

void Renderer::RenderFrameBuffer()
{
	while (true)
	{
		auto timeStart = std::chrono::system_clock::now();

		m_FrameBufferMutex.lock();
		FrameBufferRef framebuffer = m_FrameBuffers.GetRenderBuffer();
		m_FrameBufferMutex.unlock();

		m_SettingsLock.lock();
		Settings currentFrameSettings = m_Settings;
		m_SettingsLock.unlock();

		m_SceneLock.lock();
		Scene currentScene = m_Scene;
		m_SceneLock.unlock();

		Camera& camera = currentScene.camera;
		TLAS& tlas = currentScene.tlas;
		std::vector<PointLight>& pointLights = currentScene.pointLights;
		uint32_t width = camera.GetResolution().x;
		uint32_t height = camera.GetResolution().y;
		currentFrameSettings.RenderResolutionWidth = width;
		currentFrameSettings.RenderResolutionHeight = height;

		uint32_t bufferSize = width * height;
		UpdateSampleBufferSize(bufferSize);
		UpdateResevoirBufferSize(bufferSize);

		if (framebuffer->size() != width * height * 4)
		{
			framebuffer->resize(bufferSize * 4, 0);
		}

		if (currentFrameSettings.Mode != Settings::RenderMode::ReSTIR)
		{
			TaskBatch taskBatch(currentFrameSettings.ThreadCount);
			for (uint32_t y = 0; y < height; y += currentFrameSettings.RenderingKernelSize)
			{
				for (uint32_t x = 0; x < width; x += currentFrameSettings.RenderingKernelSize)
				{
					uint32_t seed = x + y * width;
					taskBatch.EnqueueTask([=]() { RenderKernelNonReSTIR(camera, framebuffer, width, height, x, y, tlas, pointLights, seed); });
				}
			}
			taskBatch.ExecuteTasks();
		}
		else
		{
			m_CurrentBuffer = (m_CurrentBuffer + 1) % 2;
			m_PrevBuffer = (m_PrevBuffer + 1) % 2;

			TaskBatch risBatch(currentFrameSettings.ThreadCount);
			for (uint32_t y = 0; y < height; y += currentFrameSettings.RenderingKernelSize)
			{
				for (uint32_t x = 0; x < width; x += currentFrameSettings.RenderingKernelSize)
				{
					risBatch.EnqueueTask([=]() { RenderKernelReSTIR(camera, framebuffer, width, height, x, y, tlas, pointLights, ReSTIRPass::RIS, x + y * width); });
				}
			}
			risBatch.ExecuteTasks();

			if (currentFrameSettings.EnableVisibilityPass)
			{
				TaskBatch visibilityBatch(currentFrameSettings.ThreadCount);
				for (uint32_t y = 0; y < height; y += currentFrameSettings.RenderingKernelSize)
				{
					for (uint32_t x = 0; x < width; x += currentFrameSettings.RenderingKernelSize)
					{
						visibilityBatch.EnqueueTask([=]() { RenderKernelReSTIR(camera, framebuffer, width, height, x, y, tlas, pointLights, ReSTIRPass::Visibility, x + y * width); });
					}
				}
				visibilityBatch.ExecuteTasks();
			}

			if (currentFrameSettings.EnableTemporalReuse)
			{
				TaskBatch TemporalBatch(currentFrameSettings.ThreadCount);
				for (uint32_t y = 0; y < height; y += currentFrameSettings.RenderingKernelSize)
				{
					for (uint32_t x = 0; x < width; x += currentFrameSettings.RenderingKernelSize)
					{
						TemporalBatch.EnqueueTask([=]() { RenderKernelReSTIR(camera, framebuffer, width, height, x, y, tlas, pointLights, ReSTIRPass::Temporal, x + y * width); });
					}
				}
				TemporalBatch.ExecuteTasks();
			}

			if (currentFrameSettings.EnableSpatialReuse)
			{
				TaskBatch SpatialBatch(currentFrameSettings.ThreadCount);
				for (uint32_t y = 0; y < height; y += currentFrameSettings.RenderingKernelSize)
				{
					for (uint32_t x = 0; x < width; x += currentFrameSettings.RenderingKernelSize)
					{
						SpatialBatch.EnqueueTask([=]() { RenderKernelReSTIR(camera, framebuffer, width, height, x, y, tlas, pointLights, ReSTIRPass::Spatial, x + y * width); });
					}
				}
				SpatialBatch.ExecuteTasks();
			}

			TaskBatch shadingBatch(currentFrameSettings.ThreadCount);
			for (uint32_t y = 0; y < height; y += currentFrameSettings.RenderingKernelSize)
			{
				for (uint32_t x = 0; x < width; x += currentFrameSettings.RenderingKernelSize)
				{
					shadingBatch.EnqueueTask([=]() { RenderKernelReSTIR(camera, framebuffer, width, height, x, y, tlas, pointLights, ReSTIRPass::Shading, x + y * width); });
				}
			}
			shadingBatch.ExecuteTasks();
		}

		auto timeEnd = std::chrono::system_clock::now();
		m_LastFrameTime = std::chrono::duration<float, std::ratio<1, 1000>>(timeEnd - timeStart).count();

		m_FrameBufferMutex.lock();
		m_FrameBuffers.SwapFrameBuffers();
		m_FrameBufferMutex.unlock();
	}
}

void Renderer::RenderKernelNonReSTIR(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const std::vector<PointLight>& pointLights, uint32_t seed)
{
	uint32_t xMax = std::min(xMin + m_Settings.RenderingKernelSize, width);
	uint32_t yMax = std::min(yMin + m_Settings.RenderingKernelSize, height);

	if (m_Settings.RandomSeed)
	{
		auto duration = std::chrono::system_clock::now().time_since_epoch();
		auto milliseconds = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
		seed += milliseconds;
	}

	for (uint32_t y = yMin; y < yMax; y++)
	{
		for (uint32_t x = xMin; x < xMax; x++)
		{
			Ray ray = camera.GetRay(x, y);
			glm::vec4 colorAccumulator = glm::vec4(0);

			switch (m_Settings.Mode)
			{
			case Settings::RenderMode::Normals:
				colorAccumulator += RenderModeNormal::RenderRay(ray, tlas, seed);
				break;
			case Settings::RenderMode::TraversalSteps:
				colorAccumulator += RenderModeTraversalSteps::RenderRay(ray, tlas, seed);
				break;
			case Settings::RenderMode::DI:
				colorAccumulator += RenderDI(ray, tlas, pointLights, seed);
				break;
			}

			Utils::FillFrameBufferPixel(x, y, colorAccumulator, width, frameBuffer);
		}
	}
}

void Renderer::RenderKernelReSTIR(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const std::vector<PointLight>& pointLights, ReSTIRPass restirPass, uint32_t seed)
{
	uint32_t xMax = std::min(xMin + m_Settings.RenderingKernelSize, width - 1);
	uint32_t yMax = std::min(yMin + m_Settings.RenderingKernelSize, height - 1);

	if (m_Settings.RandomSeed)
	{
		seed += static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	}

	switch (restirPass)
	{
	case ReSTIRPass::RIS:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				GenerateSample(camera, glm::i32vec2(x, y), x + yOffset, tlas, pointLights, seed);
			}
		}
		break;
	case ReSTIRPass::Visibility:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				VisibilityPass(m_ResevoirBuffers[m_CurrentBuffer][x + yOffset], tlas);
			}
		}
		break;
	case ReSTIRPass::Temporal:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				TemporalReuse(camera, glm::i32vec2(x, y), glm::i32vec2(width, height), x + yOffset, seed);
			}
		}
		break;
	case ReSTIRPass::Spatial:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				SpatialReuse(glm::i32vec2(x, y), glm::i32vec2(width, height), x + yOffset, seed);
			}
		}
		break;
	case ReSTIRPass::Shading:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				Utils::FillFrameBufferPixel(x, y, RenderSample(m_ResevoirBuffers[m_CurrentBuffer][x + yOffset], tlas, seed), width, frameBuffer);
			}
		}
		break;
	}
}

Sample Renderer::SamplePointLight(const Camera& camera, const glm::i32vec2& pixel, const TLAS& tlas, const std::vector<PointLight>& pointLights, uint32_t& seed)
{
	Ray ray = camera.GetRay(pixel.x, pixel.y);
	tlas.Traverse(ray); // Edge case: hitting emmisive on first vertex

	//int lightIndex = Utils::RandomInt(0, pointLights.size(), seed);
	PointLight randomPointLight = pointLights[Utils::RandomInt(0, pointLights.size(), seed)];
	glm::vec3 lightDirection = randomPointLight.position - ray.hitInfo.location;
	float lightDistance = glm::length(lightDirection);
	lightDirection = lightDirection / lightDistance;

	bool validDirectLighting = true;
	Ray shadowRay;

	// Construct Sample
	Sample sample;
	sample.valid = ray.hitInfo.hit && ray.hitInfo.material.MaterialType != Material::Type::Emissive;
	sample.Path.CameraOrigin = camera.GetTransform().translation;
	sample.Path.HitLocation = ray.hitInfo.location;
	sample.Path.LightLocation = randomPointLight.position;
	sample.Path.FirstRayHitInfo = ray.hitInfo;
	sample.Path.ShadowRayHitInfo = shadowRay.hitInfo;
	sample.Path.Light = randomPointLight;
	sample.PDF = 1.0f / pointLights.size();
	sample.Weight = pointLights.size(); //1.0f / sample.PDF; // 1 / PDF

	return sample;
}

glm::vec3 Renderer::TargetDistribution(const PathDI& path)
{
	glm::vec3 lightDirection = path.LightLocation - path.HitLocation;
	float lightDistance = glm::length(lightDirection);
	lightDirection = lightDirection / lightDistance;

	//float BRDF = glm::dot(path.FirstRayHitInfo.normal, lightDirection);
	return glm::dot(path.FirstRayHitInfo.normal, lightDirection) * path.Light.material.EmissiveIntensity * path.Light.material.EmissiveColor / (lightDistance * lightDistance);
}
void Renderer::GenerateSample(const Camera& camera, const glm::i32vec2 pixel, uint32_t bufferIndex, const TLAS& tlas, const std::vector<PointLight>& pointLights, uint32_t& seed)
{
	Resevoir<Sample> resevoir;
	Sample sample;

	for (int i = 0; i < m_Settings.CandidateCountReSTIR; i++)
	{
		sample = SamplePointLight(camera, pixel, tlas, pointLights, seed);
		//float weight = (1.0f / static_cast<float>(m_Settings.CandidateCountReSTIR)) * Utils::colorToContribution(TargetDistribution(sample.Path)) * sample.Weight;
		float weight = Utils::colorToContribution(TargetDistribution(sample.Path)) / sample.PDF;
		resevoir.Update(sample, weight, seed);
	}

	resevoir.WeightSampleOut = (1.0f / Utils::colorToContribution(TargetDistribution(resevoir.GetSampleOutRef().Path))) * (resevoir.GetWeightTotal() / resevoir.GetSampleCount());
	m_ResevoirBuffers[m_CurrentBuffer][bufferIndex] = resevoir;
}

void Renderer::VisibilityPass(Resevoir<Sample>& resevoir, const TLAS& tlas)
{
	const PathDI& path = resevoir.GetSampleOutRef().Path;
	glm::vec3 rayDirection = path.Light.position - path.FirstRayHitInfo.location;
	float rayDistance = glm::length(rayDirection);
	rayDirection = rayDirection / rayDistance;
	glm::vec3 rayOrigin = path.FirstRayHitInfo.location + m_Settings.Eta * rayDirection;

	Ray shadowRay = Ray(rayOrigin, rayDirection, rayDistance - 2.0f * m_Settings.Eta);
	if (tlas.IsOccluded(shadowRay))
		resevoir.WeightSampleOut = 0.0f;
}

void Renderer::SpatialReuse(const glm::i32vec2& pixel, const glm::i32vec2& resolution, uint32_t bufferIndex, uint32_t& seed)
{
	for (int i = 0; i < m_Settings.SpatialReuseNeighbours; i++)
	{
		glm::i32vec2 neighbour = Utils::GetNeighbourPixel(pixel, resolution, m_Settings.SpatialReuseRadius, seed);

		Resevoir<Sample>& pixelResevoir = m_ResevoirBuffers[m_CurrentBuffer][bufferIndex];
		glm::vec3 pixelHitLocation = pixelResevoir.GetSampleOutRef().Path.FirstRayHitInfo.location;
		glm::vec3 pixelHitNormal = pixelResevoir.GetSampleOutRef().Path.FirstRayHitInfo.normal;

		Resevoir<Sample>& neighbourResevoir = m_ResevoirBuffers[m_CurrentBuffer][neighbour.x + neighbour.y * resolution.x];
		glm::vec3 neighbourHitLocation = neighbourResevoir.GetSampleOutRef().Path.FirstRayHitInfo.location;
		glm::vec3 neighbourHitNormal = neighbourResevoir.GetSampleOutRef().Path.FirstRayHitInfo.normal;

		bool withinMaxDistance = glm::length(neighbourHitLocation - pixelHitLocation) < m_Settings.SpatialReuseMaxDistance;
		bool similarNormals = glm::dot(neighbourHitNormal, pixelHitNormal) >= m_Settings.SpatialReuseMinNormalSimilarity;
		// TODO: shadowray test against neighbour lightsource with pixelHitLocation
		//bool notOccluded = !tlas.isOccluded();

		if (withinMaxDistance && similarNormals) // && notOccluded
		{
			m_ResevoirBuffers[m_CurrentBuffer][bufferIndex] = CombineResevoirBiased(pixelResevoir, neighbourResevoir, seed);
			m_ResevoirBuffers[m_CurrentBuffer][bufferIndex].GetSampleOutRef().Path.HitLocation = pixelHitLocation;
			m_ResevoirBuffers[m_CurrentBuffer][bufferIndex].GetSampleOutRef().Path.FirstRayHitInfo = pixelResevoir.GetSampleOutRef().Path.FirstRayHitInfo;
		}
	}
}

void Renderer::TemporalReuse(const Camera& camera, const glm::i32vec2& pixel, const glm::i32vec2 resolution, uint32_t bufferIndex, uint32_t& seed)
{
	// Follows original paper pseudocode, does not seem to do much
	Resevoir<Sample>& currentFrameResevoir = m_ResevoirBuffers[m_CurrentBuffer][bufferIndex];
	glm::vec3 hitLocation = currentFrameResevoir.GetSampleOutRef().Path.HitLocation;

	glm::i32vec2 prevFramePixel = camera.GetPrevFramePixelCoordinates(hitLocation);
	Resevoir<Sample>& prevFrameResevoir = m_ResevoirBuffers[m_PrevBuffer][prevFramePixel.x + prevFramePixel.y * resolution.x];

	bool withinBounds = prevFramePixel.x >= 0 && prevFramePixel.y >= 0 && prevFramePixel.x < resolution.x && prevFramePixel.y < resolution.y;
	bool sameNormal = glm::dot(currentFrameResevoir.GetSampleOutRef().Path.FirstRayHitInfo.normal, prevFrameResevoir.GetSampleOutRef().Path.FirstRayHitInfo.normal) >= 0.99;
	//TODO: Check for occlusion 
	//bool notOccluded = !tlas.isOccluded();

	if (withinBounds && sameNormal) // && notOccluded
	{
		m_ResevoirBuffers[m_CurrentBuffer][bufferIndex] = CombineResevoirBiased(currentFrameResevoir, prevFrameResevoir, seed);
	}
}

glm::vec4 Renderer::RenderSample(const Resevoir<Sample>& resevoir, const TLAS& tlas, uint32_t& seed)
{
	// Direct lighting calculation
	glm::vec3 outputColor(0.0f);
	const Sample& sample = resevoir.GetSampleOut();
	const HitInfo& hitInfo = sample.Path.FirstRayHitInfo;
	const PointLight& light = sample.Path.Light;

	glm::vec3 lightDirection = sample.Path.LightLocation - sample.Path.HitLocation;
	float lightDistance = glm::length(lightDirection);
	lightDirection = glm::normalize(lightDirection);

	if (glm::dot(hitInfo.normal, lightDirection) > 0)
	{
		Ray shadowRay = Ray(hitInfo.location + (m_Settings.Eta * lightDirection), lightDirection, lightDistance - 2.0f * m_Settings.Eta);
		bool lightOccluded = tlas.IsOccluded(shadowRay);
		if (!lightOccluded || !m_Settings.LightOcclusionCheckDI)
		{
			//glm::vec3 BRDF =(hitInfo.material.Albedo / M_PI;
			float BRDF = glm::dot(hitInfo.normal, lightDirection);
			outputColor = BRDF * light.material.EmissiveIntensity * light.material.EmissiveColor / (lightDistance * lightDistance);
		}
	}

	return glm::vec4(outputColor * resevoir.WeightSampleOut, 1.0f);
}

Resevoir<Sample> Renderer::CombineResevoirBiased(const Resevoir<Sample>& originalResevoir, const Resevoir<Sample>& newResevoir, uint32_t& seed)
{
	Resevoir<Sample> combinedResevoir;

	const Sample& originalSample = originalResevoir.GetSampleOut();
	const Sample& newSample = newResevoir.GetSampleOut();

	float originalWeight = Utils::colorToContribution(TargetDistribution(originalSample.Path)) * originalResevoir.WeightSampleOut * originalResevoir.GetSampleCount();
	float newWeight = Utils::colorToContribution(TargetDistribution(newSample.Path)) * newResevoir.WeightSampleOut * newResevoir.GetSampleCount();

	combinedResevoir.Update(originalSample, originalWeight, seed);
	combinedResevoir.Update(newSample, newWeight, seed);

	combinedResevoir.SetSampleCount(originalResevoir.GetSampleCount() + newResevoir.GetSampleCount());
	combinedResevoir.WeightSampleOut = (1.0f / Utils::colorToContribution(TargetDistribution(combinedResevoir.GetSampleOutRef().Path))) * (combinedResevoir.GetWeightTotal() / combinedResevoir.GetSampleCount());

	return combinedResevoir;
}