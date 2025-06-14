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
				//glm::vec3 BRDF = ray.hitInfo.material.Albedo / M_PI;
				float BRDF = glm::dot(ray.hitInfo.normal, lightDirection);
				return BRDF * lightIntensity * lightColor / (lightDistance * lightDistance);
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
		std::vector<Sphere>& sphereLights = currentScene.sphereLights;
		uint32_t width = camera.GetResolution().x;
		uint32_t height = camera.GetResolution().y;

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
					taskBatch.EnqueueTask([=]() { RenderKernelNonReSTIR(camera, framebuffer, width, height, x, y, tlas, sphereLights, seed); });
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
					risBatch.EnqueueTask([=]() { RenderKernelReSTIR(camera, framebuffer, width, height, x, y, tlas, sphereLights, ReSTIRPass::RIS, x + y * width); });
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
						visibilityBatch.EnqueueTask([=]() { RenderKernelReSTIR(camera, framebuffer, width, height, x, y, tlas, sphereLights, ReSTIRPass::Visibility, x + y * width); });
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
						TemporalBatch.EnqueueTask([=]() { RenderKernelReSTIR(camera, framebuffer, width, height, x, y, tlas, sphereLights, ReSTIRPass::Temporal, x + y * width); });
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
						SpatialBatch.EnqueueTask([=]() { RenderKernelReSTIR(camera, framebuffer, width, height, x, y, tlas, sphereLights, ReSTIRPass::Spatial, x + y * width); });
					}
				}
				SpatialBatch.ExecuteTasks();
			}

			TaskBatch shadingBatch(currentFrameSettings.ThreadCount);
			for (uint32_t y = 0; y < height; y += currentFrameSettings.RenderingKernelSize)
			{
				for (uint32_t x = 0; x < width; x += currentFrameSettings.RenderingKernelSize)
				{
					shadingBatch.EnqueueTask([=]() { RenderKernelReSTIR(camera, framebuffer, width, height, x, y, tlas, sphereLights, ReSTIRPass::Shading, x + y * width); });
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

void Renderer::RenderKernelNonReSTIR(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t seed)
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
				colorAccumulator += RenderDI(ray, tlas, sphereLights, seed);
				break;
			}

			Utils::FillFrameBufferPixel(x, y, colorAccumulator, width, frameBuffer);
		}
	}
}

void Renderer::RenderKernelReSTIR(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const std::vector<Sphere>& sphereLights, ReSTIRPass restirPass, uint32_t seed)
{
	uint32_t xMax = std::min(xMin + m_Settings.RenderingKernelSize, width);
	uint32_t yMax = std::min(yMin + m_Settings.RenderingKernelSize, height);

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
				uint32_t bufferIndex = x + yOffset;

				m_ResevoirBuffers[m_CurrentBuffer][bufferIndex] = GenerateSample(camera, glm::i32vec2(x, y), bufferIndex, tlas, sphereLights, seed);
			}
		}
		break;
	case ReSTIRPass::Visibility:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				uint32_t bufferIndex = x + yOffset;

				VisibilityPass(m_ResevoirBuffers[m_CurrentBuffer][bufferIndex], tlas);
			}
		}
		break;
	case ReSTIRPass::Temporal:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				uint32_t bufferIndex = x + yOffset;

				TemporalReuse(camera, m_ResevoirBuffers[m_CurrentBuffer][bufferIndex], glm::i32vec2(x, y), glm::i32vec2(width, height), bufferIndex, seed);
			}
		}
		break;
	case ReSTIRPass::Spatial:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				uint32_t bufferIndex = x + yOffset;

				SpatialReuse(glm::i32vec2(x, y), glm::i32vec2(width, height), bufferIndex, seed);
			}
		}
		break;
	case ReSTIRPass::Shading:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				uint32_t bufferIndex = x + yOffset;

				Utils::FillFrameBufferPixel(x, y, RenderSample(m_ResevoirBuffers[m_CurrentBuffer][bufferIndex], tlas, seed), width, frameBuffer);
			}
		}
		break;
	}
}

Sample Renderer::SamplePointLight(const Camera& camera, const glm::i32vec2& pixel, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed)
{
	Ray ray = camera.GetRay(pixel.x, pixel.y);
	tlas.Traverse(ray); // Edge case: hitting emmisive on first vertex

	//int lightIndex = Utils::RandomInt(0, sphereLights.size(), seed);
	Sphere randomSphere = sphereLights[Utils::RandomInt(0, sphereLights.size(), seed)];
	glm::vec3 lightDirection = randomSphere.position - ray.hitInfo.location;
	float lightDistance = glm::length(lightDirection);
	lightDirection = lightDirection / lightDistance;

	bool validDirectLighting = true;
	Ray shadowRay;

	// Construct Sample
	Sample sample;
	sample.valid = ray.hitInfo.hit && ray.hitInfo.material.MaterialType != Material::Type::Emissive;
	sample.Path.CameraOrigin = camera.GetTransform().translation;
	sample.Path.HitLocation = ray.hitInfo.location;
	sample.Path.LightLocation = randomSphere.position;
	sample.Path.FirstRayHitInfo = ray.hitInfo;
	sample.Path.ShadowRayHitInfo = shadowRay.hitInfo;
	sample.Path.Light = randomSphere;
	sample.PDF = 1.0f / sphereLights.size();
	sample.Weight = sphereLights.size(); //1.0f / sample.PDF; // 1 / PDF

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

Resevoir<Sample> Renderer::GenerateSample(const Camera& camera, const glm::i32vec2 pixel, uint32_t bufferIndex, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed)
{
	Resevoir<Sample> resevoir;
	Sample sample;

	for (int i = 0; i < m_Settings.CandidateCountReSTIR; i++)
	{
		sample = SamplePointLight(camera, pixel, tlas, sphereLights, seed);
		//float weight = (1.0f / static_cast<float>(m_Settings.CandidateCountReSTIR)) * Utils::colorToContribution(TargetDistribution(sample.Path)) * sample.Weight;
		float weight = Utils::colorToContribution(TargetDistribution(sample.Path)) / sample.PDF;
		resevoir.Update(sample, weight, seed);
	}

	resevoir.WeightSampleOut = (1.0f / Utils::colorToContribution(TargetDistribution(resevoir.GetSampleOut().Path))) * (resevoir.GetWeightTotal() / resevoir.GetSampleCount());
	return resevoir;
}

void Renderer::VisibilityPass(Resevoir<Sample>& resevoir, const TLAS& tlas)
{
	PathDI path = resevoir.GetSampleOut().Path;
	glm::vec3 rayDirection = path.Light.position - path.FirstRayHitInfo.location;
	float rayDistance = glm::length(rayDirection);
	rayDirection = rayDirection / rayDistance;
	glm::vec3 rayOrigin = path.FirstRayHitInfo.location + m_Settings.Eta * rayDirection;

	Ray shadowRay = Ray(rayOrigin, rayDirection, rayDistance - 2 * m_Settings.Eta);
	if (tlas.IsOccluded(shadowRay))
		resevoir.WeightSampleOut = 0.0f;
}

// TODO: transfer light point to currentpixel path
void Renderer::SpatialReuse(const glm::i32vec2& pixel, const glm::i32vec2& resolution, uint32_t bufferIndex, uint32_t& seed)
{
	for (int i = 0; i < m_Settings.SpatialReuseIterationCount; i++)
	{
		std::vector<glm::i32vec2> neighbours = Utils::GetNeighbourPixels(pixel, resolution, m_Settings.SpatialReuseNeighbours, m_Settings.SpatialReuseRadius, seed);
		std::vector<Resevoir<Sample>> resevoirs;

		Resevoir<Sample> pixelResevoir = m_ResevoirBuffers[m_CurrentBuffer][pixel.x + pixel.y * resolution.x];
		glm::vec3 pixelHitLocation = pixelResevoir.GetSampleOut().Path.FirstRayHitInfo.location;
		glm::vec3 pixelHitNormal = pixelResevoir.GetSampleOut().Path.FirstRayHitInfo.normal;

		for (glm::i32vec2 neighbour : neighbours)
		{
			Resevoir<Sample> neighbourResevoir = m_ResevoirBuffers[m_CurrentBuffer][neighbour.x + neighbour.y * resolution.x];
			glm::vec3 neighbourHitLocation = neighbourResevoir.GetSampleOut().Path.FirstRayHitInfo.location;
			glm::vec3 neighbourHitNormal = neighbourResevoir.GetSampleOut().Path.FirstRayHitInfo.normal;

			bool withinMaxDistance = glm::length(neighbourHitLocation - pixelHitLocation) < m_Settings.SpatialReuseMaxDistance;
			bool similarNormals = glm::dot(neighbourHitNormal, pixelHitNormal) >= m_Settings.SpatialReuseMinNormalSimilarity;

			if (withinMaxDistance && similarNormals)
				resevoirs.push_back(neighbourResevoir);
		}

		Resevoir<Sample> newSample = CombineResevoirsBiased(m_ResevoirBuffers[m_CurrentBuffer][bufferIndex], resevoirs, seed);
		newSample.GetSampleOutRef().Path.HitLocation = pixelHitLocation;
		newSample.GetSampleOutRef().Path.FirstRayHitInfo = pixelResevoir.GetSampleOut().Path.FirstRayHitInfo;
		m_ResevoirBuffers[m_CurrentBuffer][bufferIndex] = newSample;
	}
}

void Renderer::TemporalReuse(const Camera& camera, const Resevoir<Sample>& resevoir, const glm::i32vec2& pixel, const glm::i32vec2 resolution, uint32_t bufferIndex, uint32_t& seed)
{
	// Follows original paper pseudocode, does not seem to do much
	Resevoir<Sample> currentFrameResevoir = m_ResevoirBuffers[m_CurrentBuffer][bufferIndex];
	
	glm::vec3 hitLocation = resevoir.GetSampleOut().Path.HitLocation;
	glm::i32vec2 prevFramePixel = camera.GetPrevFramePixelCoordinates(hitLocation);
	Resevoir<Sample> prevFrameResevoir = m_ResevoirBuffers[m_PrevBuffer][prevFramePixel.x + prevFramePixel.y * resolution.x];
	bool withinBounds = prevFramePixel.x >= 0 && prevFramePixel.y >= 0 && prevFramePixel.x < resolution.x && prevFramePixel.y < resolution.y;
	bool sameNormal = glm::dot(resevoir.GetSampleOut().Path.FirstRayHitInfo.normal, prevFrameResevoir.GetSampleOut().Path.FirstRayHitInfo.normal) >= 0.99;
	
	if (withinBounds && sameNormal)
	{
		std::vector<Resevoir<Sample>> prevFrameResevoir;
		prevFrameResevoir.push_back(m_ResevoirBuffers[m_PrevBuffer][prevFramePixel.x + prevFramePixel.y * resolution.x]);
		m_ResevoirBuffers[m_CurrentBuffer][bufferIndex] = CombineResevoirsBiased(currentFrameResevoir, prevFrameResevoir, seed);
	}

	// A gentle introduction to Restir method

	//glm::vec3 hitLocation = resevoir.GetSampleOut().Path.HitLocation;
	//glm::i32vec2 prevFramePixel = camera.GetPrevFramePixelCoordinates(hitLocation);
	//bool withinBounds = prevFramePixel.x >= 0 && prevFramePixel.y >= 0 && prevFramePixel.x < resolution.x && prevFramePixel.y < resolution.y;
	//
	//if (withinBounds)
	//{
	//	Resevoir<Sample> temporalResevoir;
	//
	//	Sample currentSample = resevoir.GetSampleOut();
	//	Sample prevSample = m_ResevoirBuffers[m_PrevBuffer][prevFramePixel.x + prevFramePixel.y * resolution.x].GetSampleOut();
	//
	//	float contributionCurrent = Utils::colorToContribution(TargetDistribution(currentSample.Path));
	//	float contributionPrev = Utils::colorToContribution(TargetDistribution(prevSample.Path));
	//	float MISWeightCurrent = contributionCurrent / (20 * contributionPrev + contributionCurrent);
	//	float MISWeightPrev = (20 * contributionPrev) / (20 * contributionPrev + contributionCurrent);
	//
	//	float weightCurrent = MISWeightCurrent * contributionCurrent * currentSample.Weight;
	//	temporalResevoir.Update(currentSample, weightCurrent, seed);
	//
	//	float weightPrev = MISWeightPrev * contributionPrev * prevSample.Weight;
	//	temporalResevoir.Update(prevSample, weightPrev, seed);
	//
	//	Sample sampleOut = temporalResevoir.GetSampleOut();
	//	temporalResevoir.WeightSampleOut = 1.0f / Utils::colorToContribution(TargetDistribution(sampleOut.Path)) * temporalResevoir.GetWeightTotal();
	//	m_ResevoirBuffers[m_CurrentBuffer][bufferIndex] = temporalResevoir;
	//}
}

glm::vec4 Renderer::RenderSample(const Resevoir<Sample>& resevoir, const TLAS& tlas, uint32_t& seed)
{
	// Direct lighting calculation
	glm::vec3 outputColor(0.0f);
	Sample sample = resevoir.GetSampleOut();
	HitInfo hitInfo = sample.Path.FirstRayHitInfo;
	Sphere light = sample.Path.Light;

	glm::vec3 lightDirection = sample.Path.LightLocation - sample.Path.HitLocation;
	float lightDistance = glm::length(lightDirection);
	lightDirection = glm::normalize(lightDirection);

	if (glm::dot(hitInfo.normal, lightDirection) > 0)
	{
		Ray shadowRay = Ray(hitInfo.location + (m_Settings.Eta * lightDirection), lightDirection, lightDistance - 2 * m_Settings.Eta);
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

Resevoir<Sample> Renderer::CombineResevoirsBiased(const Resevoir<Sample>& originalResevoir, std::vector<Resevoir<Sample>>& newResevoirs, uint32_t& seed)
{
	Resevoir<Sample> combinedResevoir;

	int totalCandidates = 0;
	newResevoirs.push_back(originalResevoir);
	for (int i = 0; i < newResevoirs.size(); i++)
	{
		Resevoir<Sample> currentResevoir = newResevoirs[i];
		Sample sample = currentResevoir.GetSampleOut();
		float weight = Utils::colorToContribution(TargetDistribution(sample.Path)) * currentResevoir.WeightSampleOut * currentResevoir.GetSampleCount();
		combinedResevoir.Update(sample, weight, seed);

		totalCandidates += currentResevoir.GetSampleCount();
	}

	combinedResevoir.SetSampleCount(totalCandidates);
	combinedResevoir.WeightSampleOut = (1.0f / Utils::colorToContribution(TargetDistribution(combinedResevoir.GetSampleOut().Path))) * (combinedResevoir.GetWeightTotal() / combinedResevoir.GetSampleCount());

	return combinedResevoir;
}