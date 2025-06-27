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
			E = 0.5f * ray.hitInfo.normal + 0.5f;

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
			E = 0.5f * (1.0f / 100.0f) * glm::vec3(ray.hitInfo.traversalStepsHitBVH) + 0.5f;

		return glm::vec4(E, 1.0f);
	}
}


glm::vec4 Renderer::RenderDI(Ray& ray, uint32_t& seed)
{
	glm::vec3 E(0.0f);
	
	// Intersection Test
	m_Scene.tlas.Traverse(ray);
	if (!ray.hitInfo.hit)
		return glm::vec4(0.8f, 0.2f, 0.8f, 1.0f);

	auto CalcLightContribution = [=](const Ray& ray, const PointLight& pointLight) {
		glm::vec3 lightDirection = pointLight.position - ray.hitInfo.location;
		float lightDistance = glm::length(lightDirection);
		lightDirection = glm::normalize(lightDirection);

		if (glm::dot(ray.hitInfo.normal, lightDirection) > 0)
		{
			Ray shadowRay = Ray(ray.hitInfo.location + (m_Settings.Eta * lightDirection), lightDirection, lightDistance - 2 * m_Settings.Eta);
			bool lightOccluded = m_Scene.tlas.IsOccluded(shadowRay);
			if (!lightOccluded || !m_Settings.LightOcclusionCheckDI)
			{
				//glm::vec3 BRDF = ray.hitInfo.material.Albedo / M_PI;
				float BRDF = glm::dot(ray.hitInfo.normal, lightDirection);
				return BRDF * pointLight.emmission / (lightDistance * lightDistance);
			}
		}

		return glm::vec3(0);
	};

	if (m_Settings.SampleAllLightsDI)
	{
		for (int i = 0; i < m_Scene.pointLights.size(); i++)
		{
			E += CalcLightContribution(ray, m_Scene.pointLights[i]);
		}
	}
	else {
		for (int i = 0; i < m_Settings.CandidateCountDI; i++)
		{
			int index = Utils::RandomInt(0, m_Scene.pointLights.size(), seed);
			PointLight randomPointLight = m_Scene.pointLights[index];
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

		if (SettingsUpdated)
		{
			m_SettingsLock.lock();

			if (m_Settings != m_NewSettings)
				m_ValidHistory = false;

			m_Settings = m_NewSettings;
			SettingsUpdated = false;
			m_SettingsLock.unlock();
		}

		if (SceneUpdated)
		{
			m_SceneLock.lock();
			m_Scene = m_NewScene;
			m_SceneLock.unlock();

			m_Scene.camera.UpdateFrustrum();
			m_Scene.camera.UpdateCameraMatrix();
		}

		uint32_t width = m_Settings.RenderResolutionWidth;
		uint32_t height = m_Settings.RenderResolutionHeight;

		uint32_t bufferSize = width * height;
		UpdateSampleBufferSize(bufferSize);
		UpdateResevoirBufferSize(bufferSize);

		if (framebuffer->size() != width * height * 4)
		{
			framebuffer->resize(bufferSize * 4, 0);
		}

		if (m_Settings.Mode != Settings::RenderMode::ReSTIR)
		{
			TaskBatch taskBatch(m_Settings.ThreadCount);
			for (uint32_t y = 0; y < height; y += m_Settings.RenderingKernelSize)
			{
				for (uint32_t x = 0; x < width; x += m_Settings.RenderingKernelSize)
				{
					uint32_t seed = x + y * width;
					taskBatch.EnqueueTask([=]() {RenderKernelNonReSTIR(framebuffer, width, height, x, y, seed); });
				}
			}
			taskBatch.ExecuteTasks();
		}
		else
		{
			m_CurrentBuffer = (m_CurrentBuffer + 1) % 2;
			m_PrevBuffer = (m_PrevBuffer + 1) % 2;

			TaskBatch risBatch(m_Settings.ThreadCount);
			for (uint32_t y = 0; y < height; y += m_Settings.RenderingKernelSize)
			{
				for (uint32_t x = 0; x < width; x += m_Settings.RenderingKernelSize)
				{
					risBatch.EnqueueTask([=]() { RenderKernelReSTIR(framebuffer, width, height, x, y, ReSTIRPass::RIS, x + y * width); });
				}
			}
			risBatch.ExecuteTasks();

			if (m_Settings.EnableVisibilityPass)
			{
				TaskBatch visibilityBatch(m_Settings.ThreadCount);
				for (uint32_t y = 0; y < height; y += m_Settings.RenderingKernelSize)
				{
					for (uint32_t x = 0; x < width; x += m_Settings.RenderingKernelSize)
					{
						visibilityBatch.EnqueueTask([=]() { RenderKernelReSTIR(framebuffer, width, height, x, y, ReSTIRPass::Visibility, x + y * width); });
					}
				}
				visibilityBatch.ExecuteTasks();
			}

			if (m_Settings.EnableTemporalReuse)
			{
				TaskBatch TemporalBatch(m_Settings.ThreadCount);
				for (uint32_t y = 0; y < height; y += m_Settings.RenderingKernelSize)
				{
					for (uint32_t x = 0; x < width; x += m_Settings.RenderingKernelSize)
					{
						TemporalBatch.EnqueueTask([=]() { RenderKernelReSTIR(framebuffer, width, height, x, y, ReSTIRPass::Temporal, x + y * width); });
					}
				}
				TemporalBatch.ExecuteTasks();
			}

			if (m_Settings.EnableSpatialReuse)
			{
				TaskBatch SpatialBatch(m_Settings.ThreadCount);
				for (uint32_t y = 0; y < height; y += m_Settings.RenderingKernelSize)
				{
					for (uint32_t x = 0; x < width; x += m_Settings.RenderingKernelSize)
					{
						SpatialBatch.EnqueueTask([=]() { RenderKernelReSTIR(framebuffer, width, height, x, y, ReSTIRPass::Spatial, x + y * width); });
					}
				}
				SpatialBatch.ExecuteTasks();
			}

			TaskBatch shadingBatch(m_Settings.ThreadCount);
			for (uint32_t y = 0; y < height; y += m_Settings.RenderingKernelSize)
			{
				for (uint32_t x = 0; x < width; x += m_Settings.RenderingKernelSize)
				{
					shadingBatch.EnqueueTask([=]() { RenderKernelReSTIR(framebuffer, width, height, x, y, ReSTIRPass::Shading, x + y * width); });
				}
			}
			shadingBatch.ExecuteTasks();
		}

		auto timeEnd = std::chrono::system_clock::now();
		m_LastFrameTime = std::chrono::duration<float, std::ratio<1, 1000>>(timeEnd - timeStart).count();

		m_FrameBufferMutex.lock();
		m_FrameBuffers.SwapFrameBuffers();
		m_FrameBufferMutex.unlock();

		m_ValidHistory = true;
	}
}

void Renderer::RenderKernelNonReSTIR(FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, uint32_t seed)
{
	uint32_t xMax = std::min(xMin + m_Settings.RenderingKernelSize, width);
	uint32_t yMax = std::min(yMin + m_Settings.RenderingKernelSize, height);

	if (m_Settings.RandomSeed)
	{
		auto duration = std::chrono::system_clock::now().time_since_epoch();
		auto milliseconds = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
		seed += milliseconds;
	}

	glm::vec4 colorAccumulator;

	switch (m_Settings.Mode)
	{
	case Settings::RenderMode::Normals:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			for (uint32_t x = xMin; x < xMax; x++)
			{
				Ray ray = m_Scene.camera.GetRay(x, y);
				colorAccumulator = RenderModeNormal::RenderRay(ray, m_Scene.tlas, seed);

				Utils::FillFrameBufferPixel(x, y, colorAccumulator, width, frameBuffer);
			}
		}
		break;
	case Settings::RenderMode::TraversalSteps:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			for (uint32_t x = xMin; x < xMax; x++)
			{
				Ray ray = m_Scene.camera.GetRay(x, y);
				colorAccumulator = RenderModeTraversalSteps::RenderRay(ray, m_Scene.tlas, seed);

				Utils::FillFrameBufferPixel(x, y, colorAccumulator, width, frameBuffer);
			}
		}
		break;
	case Settings::RenderMode::DI:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			for (uint32_t x = xMin; x < xMax; x++)
			{
				Ray ray = m_Scene.camera.GetRay(x, y);
				colorAccumulator = RenderDI(ray, seed);

				Utils::FillFrameBufferPixel(x, y, colorAccumulator, width, frameBuffer);
			}
		}
		break;
	}

}

void Renderer::RenderKernelReSTIR(FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, ReSTIRPass restirPass, uint32_t seed)
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
				GenerateSample(glm::i32vec2(x, y), x + yOffset, seed);
			}
		}
		break;
	case ReSTIRPass::Visibility:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				VisibilityPass(m_ResevoirBuffers[m_CurrentBuffer][x + yOffset]);
			}
		}
		break;
	case ReSTIRPass::Temporal:
		if (m_ValidHistory)
		{
			for (uint32_t y = yMin; y < yMax; y++)
			{
				uint32_t yOffset = y * width;
				for (uint32_t x = xMin; x < xMax; x++)
				{
					TemporalReuse(glm::i32vec2(x, y), glm::i32vec2(width, height), x + yOffset, seed);
				}
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
				Utils::FillFrameBufferPixel(x, y, RenderSample(m_ResevoirBuffers[m_CurrentBuffer][x + yOffset], seed), width, frameBuffer);
			}
		}
		break;
	}
}

Sample Renderer::SamplePointLight(const glm::i32vec2& pixel, uint32_t& seed)
{
	Ray ray = m_Scene.camera.GetRay(pixel.x, pixel.y);
	m_Scene.tlas.Traverse(ray); // Edge case: hitting emmisive on first vertex

	PointLight randomPointLight = m_Scene.pointLights[Utils::RandomInt(0, m_Scene.pointLights.size(), seed)];
	glm::vec3 lightDirection = randomPointLight.position - ray.hitInfo.location;
	float lightDistance = glm::length(lightDirection);
	lightDirection = lightDirection / lightDistance;

	bool validDirectLighting = true;
	Ray shadowRay;

	// Construct Sample
	Sample sample;
	sample.Path.CameraOrigin = m_Scene.camera.GetTransform().translation;
	sample.Path.hitInfo = ray.hitInfo;
	sample.Path.Light = randomPointLight;
	sample.PDF = 1.0f / m_Scene.pointLights.size();
	sample.Weight = m_Scene.pointLights.size(); //1.0f / sample.PDF; // 1 / PDF

	return sample;
}

glm::vec3 Renderer::TargetDistribution(const PathDI& path)
{
	glm::vec3 directionToLight = path.Light.position - path.hitInfo.location;
	float lightDistance = glm::length(directionToLight);
	directionToLight = directionToLight / lightDistance;

	//float BRDF = glm::dot(path.hitInfo.normal, directionToLight);
	return glm::dot(path.hitInfo.normal, directionToLight) * path.Light.emmission / (lightDistance * lightDistance);
}

void Renderer::GenerateSample(const glm::i32vec2 pixel, uint32_t bufferIndex, uint32_t& seed)
{
	Resevoir resevoir;
	Sample sample;

	for (int i = 0; i < m_Settings.CandidateCountReSTIR; i++)
	{
		sample = SamplePointLight(pixel, seed);
		float weight = Utils::colorToContribution(TargetDistribution(sample.Path)) / sample.PDF;
		resevoir.Update(sample, weight, seed);
	}

	resevoir.WeightSampleOut = (1.0f / Utils::colorToContribution(TargetDistribution(resevoir.GetSampleRef().Path))) * (resevoir.GetWeightTotal() / resevoir.GetSampleCount());
	m_ResevoirBuffers[m_CurrentBuffer][bufferIndex] = resevoir;
}

void Renderer::VisibilityPass(Resevoir& resevoir)
{
	const PathDI& path = resevoir.GetSampleRef().Path;

	if (!path.hitInfo.hit)
	{
		resevoir.WeightSampleOut = 0.0f;
		return;
	}

	glm::vec3 rayDirection = path.Light.position - path.hitInfo.location;
	float rayDistance = glm::length(rayDirection);
	rayDirection = rayDirection / rayDistance;
	glm::vec3 rayOrigin = path.hitInfo.location + m_Settings.Eta * rayDirection;

	Ray shadowRay = Ray(rayOrigin, rayDirection, rayDistance - 2.0f * m_Settings.Eta);
	if (m_Scene.tlas.IsOccluded(shadowRay))
		resevoir.WeightSampleOut = 0.0f;
}

void Renderer::SpatialReuse(const glm::i32vec2& pixel, const glm::i32vec2& resolution, uint32_t bufferIndex, uint32_t& seed)
{
	for (int i = 0; i < m_Settings.SpatialReuseNeighbours; i++) //TODO: make spatialreuse write to another buffer
	{
		glm::i32vec2 neighbour = Utils::GetNeighbourPixel(pixel, resolution, m_Settings.SpatialReuseRadius, seed);

		Resevoir& pixelResevoir = m_ResevoirBuffers[m_CurrentBuffer][bufferIndex];
		const Sample& pixelSample = pixelResevoir.GetSampleRef();
		const glm::vec3& pixelHitLocation = pixelSample.Path.hitInfo.location;
		const glm::vec3& pixelHitNormal = pixelSample.Path.hitInfo.normal;

		Resevoir& neighbourResevoir = m_ResevoirBuffers[m_CurrentBuffer][neighbour.x + neighbour.y * resolution.x];
		const Sample& neighbourSample = neighbourResevoir.GetSampleRef();
		const glm::vec3& neighbourHitLocation = neighbourSample.Path.hitInfo.location;
		const glm::vec3& neighbourHitNormal = neighbourSample.Path.hitInfo.normal;

		bool notOccluded = false;
		if (pixelSample.Path.hitInfo.hit)
		{
			glm::vec3 testDirection = neighbourSample.Path.Light.position - pixelHitLocation;
			float testDistance = glm::length(testDirection);
			testDirection /= testDistance;
			glm::vec3 testOrigin = pixelHitLocation + m_Settings.Eta * testDirection;
			notOccluded = !m_Scene.tlas.IsOccluded(Ray(testOrigin, testDirection, testDistance - 2 * m_Settings.Eta));
		}

		bool withinMaxDistance = glm::length(neighbourHitLocation - pixelHitLocation) < m_Settings.SpatialReuseMaxDistance;
		bool similarNormals = glm::dot(neighbourHitNormal, pixelHitNormal) >= m_Settings.SpatialReuseMinNormalSimilarity;
		bool validSample = neighbourSample.Weight > 0 && !std::isnan(neighbourSample.Weight);

		if (withinMaxDistance && similarNormals && notOccluded && validSample)
		{
			Resevoir newResevoir = m_ResevoirBuffers[m_CurrentBuffer][neighbour.x + neighbour.y * resolution.x];
			newResevoir.GetSampleRef().Path.hitInfo = pixelSample.Path.hitInfo;

			m_ResevoirBuffers[m_CurrentBuffer][bufferIndex] = CombineResevoirBiased(pixelResevoir, newResevoir, seed);
		}
	}
}

void Renderer::TemporalReuse(const glm::i32vec2& pixel, const glm::i32vec2 resolution, uint32_t bufferIndex, uint32_t& seed)
{
	// Follows original paper pseudocode, does not seem to do much
	Resevoir& currentFrameResevoir = m_ResevoirBuffers[m_CurrentBuffer][bufferIndex];
	HitInfo currentFrameHitInfo = m_ResevoirBuffers[m_CurrentBuffer][bufferIndex].GetSample().Path.hitInfo;
	glm::vec3 hitLocation = currentFrameResevoir.GetSampleRef().Path.hitInfo.location;

	glm::i32vec2 prevFramePixel = m_Scene.camera.GetPrevFramePixelCoordinates(hitLocation);
	//TODO: check for issues with this line
	Resevoir& prevFrameResevoir = m_ResevoirBuffers[m_PrevBuffer][prevFramePixel.x + prevFramePixel.y * resolution.x];

	bool withinBounds = prevFramePixel.x >= 0 && prevFramePixel.y >= 0 && prevFramePixel.x < resolution.x && prevFramePixel.y < resolution.y;
	bool sameNormal = glm::dot(currentFrameResevoir.GetSampleRef().Path.hitInfo.normal, prevFrameResevoir.GetSampleRef().Path.hitInfo.normal) >= m_Settings.TemporalReuseMinNormalSimilarity;
	bool withinMaxDistance = glm::length(hitLocation - prevFrameResevoir.GetSampleRef().Path.hitInfo.location) < m_Settings.TemporalReuseMaxDistance;

	bool notOccluded = false;
	if (currentFrameResevoir.GetSampleRef().Path.hitInfo.hit)
	{
		glm::vec3 testDirection = prevFrameResevoir.GetSampleRef().Path.hitInfo.location - m_Scene.camera.transform.translation;
		float testDistance = glm::length(testDirection);
		testDirection = glm::normalize(testDirection);
		glm::vec3 testOrigin = m_Scene.camera.transform.translation + m_Settings.Eta * testDirection;
		notOccluded = !m_Scene.tlas.IsOccluded(Ray(testOrigin, testDirection, testDistance - 2 * m_Settings.Eta));
	}

	// TODO: improve prevFrame lookup to reduce errors
	if (withinMaxDistance && withinBounds && sameNormal && notOccluded)
	{
		m_ResevoirBuffers[m_CurrentBuffer][bufferIndex] = CombineResevoirBiased(currentFrameResevoir, prevFrameResevoir, seed);
		m_ResevoirBuffers[m_CurrentBuffer][bufferIndex].GetSampleRef().Path.hitInfo.location = currentFrameHitInfo.location;
		m_ResevoirBuffers[m_CurrentBuffer][bufferIndex].GetSampleRef().Path.hitInfo.distance = currentFrameHitInfo.distance;
		//m_ResevoirBuffers[m_CurrentBuffer][bufferIndex].GetSampleRef().Path.hitInfo.normal = currentFrameHitInfo.normal; // TODO: find why this does not work!
		m_ResevoirBuffers[m_CurrentBuffer][bufferIndex].GetSampleRef().Path.CameraOrigin = m_Scene.camera.transform.translation;
	}
	else
	{
		//m_ResevoirBuffers[m_CurrentBuffer][bufferIndex].WeightSampleOut = 100.0f;
	}
}

glm::vec4 Renderer::RenderSample(const Resevoir& resevoir, uint32_t& seed)
{
	// Direct lighting calculation
	glm::vec3 outputColor(0.0f);
	const Sample& sample = resevoir.GetSample();
	const HitInfo& hitInfo = sample.Path.hitInfo;
	const PointLight& light = sample.Path.Light;

	glm::vec3 lightDirection = sample.Path.Light.position - sample.Path.hitInfo.location;
	float lightDistance = glm::length(lightDirection);
	lightDirection = glm::normalize(lightDirection);

	if (glm::dot(hitInfo.normal, lightDirection) > 0)
	{
		Ray shadowRay = Ray(hitInfo.location + (m_Settings.Eta * lightDirection), lightDirection, lightDistance - 2.0f * m_Settings.Eta);
		bool lightOccluded = m_Scene.tlas.IsOccluded(shadowRay);
		if (!lightOccluded || !m_Settings.LightOcclusionCheckDI)
		{
			float BRDF = glm::dot(hitInfo.normal, lightDirection);
			outputColor = BRDF * light.emmission / (lightDistance * lightDistance);
		}
	}

	//if (outputColor == glm::vec3(0))
	//	return glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);

	return glm::vec4(outputColor * resevoir.WeightSampleOut, 1.0f);
}

Resevoir Renderer::CombineResevoirBiased(const Resevoir& originalResevoir, const Resevoir& newResevoir, uint32_t& seed)
{
	Resevoir combinedResevoir;

	const Sample& originalSample = originalResevoir.GetSample();
	const Sample& newSample = newResevoir.GetSample();

	float originalWeight = Utils::colorToContribution(TargetDistribution(originalSample.Path)) * originalResevoir.WeightSampleOut * originalResevoir.GetSampleCount();
	float newWeight = Utils::colorToContribution(TargetDistribution(newSample.Path)) * newResevoir.WeightSampleOut * newResevoir.GetSampleCount();

	combinedResevoir.Update(originalSample, originalWeight, seed);
	combinedResevoir.Update(newSample, newWeight, seed);

	combinedResevoir.SetSampleCount(originalResevoir.GetSampleCount() + newResevoir.GetSampleCount());
	combinedResevoir.WeightSampleOut = (1.0f / Utils::colorToContribution(TargetDistribution(combinedResevoir.GetSampleRef().Path))) * (combinedResevoir.GetWeightTotal() / combinedResevoir.GetSampleCount());

	return combinedResevoir;
}