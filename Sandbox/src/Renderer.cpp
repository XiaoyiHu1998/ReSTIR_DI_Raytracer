#include "Renderer.h"

#include <chrono>
#include <functional>

#include "Utils.h"
#include "TaskBatch.h"
#include "RenderCommand.h"

// ================= Normal Rendering mode =================

namespace RenderModeNormal
{
	glm::vec4 RenderRay(Ray& ray, const TLAS& tlas, const RendererSettings& settings, uint32_t& seed)
	{
		glm::vec3 E(0.1f);

		tlas.Traverse(ray);
		if (ray.hitInfo.hit)
		{
			glm::vec3 normal = settings.RenderPrevNormals ? ray.hitInfo.prevNormal : ray.hitInfo.normal;
			E = 0.5f * normal + 0.5f;
		}

		return glm::vec4(E, 1.0f);
	}
}

// ================= TraversalSteps Rendering Mode =================

namespace RenderModeTraversalSteps
{
	glm::vec4 RenderRay(Ray& ray, const TLAS& tlas, const RendererSettings& settings, uint32_t& seed)
	{
		glm::vec3 E(0.1f);

		tlas.Traverse(ray);
		if (ray.hitInfo.hit)
			E = glm::vec3(static_cast<float>(ray.hitInfo.traversalStepsHitBVH) * 0.025f) + 0.1f;

		return glm::vec4(E, 1.0f);
	}
}

// ================= Next Event estimation DI rendering mode =================

glm::vec4 Renderer::RenderDI(Ray& ray, uint32_t& seed)
{
	glm::vec3 E(0.0f);
	
	// Intersection Test
	m_Scene.tlas.Traverse(ray);
	if (!ray.hitInfo.hit)
		return glm::vec4(0.8f, 0.2f, 0.8f, 1.0f);

	auto CalcLightContribution = [=](const Ray& ray, const PointLight& pointLight) {
		glm::vec3 lightDirection = pointLight.position - ray.hitInfo.position;
		float lightDistance = glm::length(lightDirection);
		lightDirection = glm::normalize(lightDirection);

		if (glm::dot(ray.hitInfo.normal, lightDirection) > 0)
		{
			Ray shadowRay = Ray(ray.hitInfo.position + (m_Settings.Eta * lightDirection), lightDirection, lightDistance - 2 * m_Settings.Eta);
			bool lightOccluded = m_Scene.tlas.IsOccluded(shadowRay);
			if (!lightOccluded || !m_Settings.OcclusionCheckDI)
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

		E *= m_Scene.pointLights.size() / m_Settings.CandidateCountDI;
	}

	return glm::vec4(E, 1.0f);
}

// ================= ReSTIR rendering mode =================

void Renderer::GenerateSample(const glm::i32vec2 pixel, uint32_t bufferIndex, uint32_t& seed)
{
	Resevoir resevoir;
	Sample sample;

	for (int i = 0; i < m_Settings.CandidateCountReSTIR; i++)
	{
		PointLight randomPointLight = m_Scene.pointLights[Utils::RandomInt(0, m_Scene.pointLights.size(), seed)];
		Ray ray = m_Scene.camera.GetRay(pixel.x, pixel.y);
		m_Scene.tlas.Traverse(ray);

		sample = Sample(ray.hitInfo, m_Scene.camera.position, randomPointLight, m_Scene.pointLights.size(), 1.0f / m_Scene.pointLights.size());
		float weight = sample.contribution / sample.pdf;
		resevoir.Update(sample, weight, seed);
	}

	resevoir.WeightSampleOut = (1.0f / resevoir.GetSampleRef().contribution) * (resevoir.GetWeightTotal() / resevoir.GetSampleCount());
	m_ResevoirBuffers.GetCurrentBuffer()[bufferIndex] = resevoir;
}

void Renderer::VisibilityPass(uint32_t bufferIndex)
{
	Resevoir& resevoir = m_ResevoirBuffers.GetCurrentBuffer()[bufferIndex];
	const Sample& sample = m_ResevoirBuffers.GetCurrentBuffer()[bufferIndex].GetSampleRef();

	if (!sample.hit || glm::dot(glm::normalize(sample.lightDirection), sample.hitNormal) < 0.001f)
	{
		resevoir.WeightSampleOut = 0.0f;
		return;
	}

	glm::vec3 rayOrigin = sample.hitPosition + m_Settings.Eta * sample.lightDirection;
	Ray shadowRay = Ray(rayOrigin, sample.lightDirection, sample.lightDistance - 2.0f * m_Settings.Eta);

	if (m_Scene.tlas.IsOccluded(shadowRay))
		resevoir.WeightSampleOut = 0.0f;
}

void Renderer::TemporalReuse(const glm::i32vec2& pixel, const glm::i32vec2 resolution, uint32_t bufferIndex, uint32_t& seed)
{
	const Resevoir& pixelResevoir = m_ResevoirBuffers.GetCurrentBuffer()[bufferIndex];
	Sample pixelSample = pixelResevoir.GetSample();

	glm::i32vec2 prevPixel = m_PrevCamera.WorldSpaceToScreenSpace(pixelSample.hitPrevPosition, seed);
	bool withinFrame = prevPixel.x >= 0 && prevPixel.y >= 0 && prevPixel.x < resolution.x && prevPixel.y < resolution.y;
	if (!withinFrame || !m_ValidHistory)
		return;

	Resevoir& prevResevoir = m_ResevoirBuffers.GetPrevBuffer()[prevPixel.x + prevPixel.y * resolution.x];
	const Sample& prevSample = prevResevoir.GetSample();

	if (!prevSample.hit)
		return;

	float cameraDistance = glm::length(pixelSample.hitPosition - m_Scene.camera.position);
	// Grow maxDistance with camera distance to make sure distant pixels don't always exceed maxDistance
	float scaledMaxDistance = m_Settings.TemporalMaxDistance + (cameraDistance * m_Settings.TemporalMaxDistanceDepthScaling);
	bool withinMaxDistance = glm::length(prevSample.hitPosition - pixelSample.hitPrevPosition) <= scaledMaxDistance;
	bool sameNormals = glm::dot(prevSample.hitNormal, pixelSample.hitPrevNormal) >= m_Settings.TemporalMinNormalSimilarity;

	glm::vec3 shadowRayDirection = prevSample.light.position - pixelSample.hitPosition;
	float shadowRayDistance = glm::length(shadowRayDirection);
	shadowRayDirection = glm::normalize(shadowRayDirection);
	glm::vec3 shadowRayOrigin = pixelSample.hitPosition + m_Settings.Eta * shadowRayDirection;
	bool notOccluded = !m_Scene.tlas.IsOccluded(Ray(shadowRayOrigin, shadowRayDirection, shadowRayDistance - 2 * m_Settings.Eta));

	if (withinMaxDistance && sameNormals && notOccluded && prevResevoir.WeightSampleOut > 0.01f)
	{
		// Limit Temporal propogation
		prevResevoir.SetSampleCount(std::min(m_Settings.TemporalSampleCountRatio * pixelResevoir.GetSampleCount(), prevResevoir.GetSampleCount()));

		Resevoir temporalResevoir = Resevoir::CombineBiased(pixelResevoir, prevResevoir, seed);
		pixelSample.ReplaceLight(temporalResevoir.GetSample().light);
		temporalResevoir.SetSample(pixelSample);
		m_ResevoirBuffers.GetCurrentBuffer()[bufferIndex] = temporalResevoir;
	}
}

void Renderer::CombineNeighbourPixel(Resevoir& pixelResevoir, const glm::i32vec2 pixel, const glm::i32vec2& resolution, uint32_t& seed)
{
	Sample pixelSample = pixelResevoir.GetSample();

	glm::i32vec2 neighbourPixel = Utils::GetNeighbourPixel(pixel, resolution, m_Settings.SpatialPixelRadius, seed);
	Resevoir& neighbourResevoir = m_ResevoirBuffers.GetCurrentBuffer()[neighbourPixel.x + neighbourPixel.y * resolution.x];
	const Sample& neighbourSample = neighbourResevoir.GetSampleRef();

	if (!neighbourSample.hit)
		return;

	float cameraDistance = glm::length(pixelSample.hitPosition - m_Scene.camera.position);
	// Grow maxDistance with camera distance to make sure distant pixels don't always exceed maxDistance
	float scaledMaxDistance = m_Settings.SpatialMaxDistance + (cameraDistance * m_Settings.SpatialMaxDistanceDepthScaling);
	bool withinMaxDistance = glm::length(neighbourSample.hitPosition - pixelSample.hitPosition) <= scaledMaxDistance;
	bool sameNormals = glm::dot(pixelSample.hitNormal, neighbourSample.hitNormal) >= m_Settings.SpatialMinNormalSimilarity;

	glm::vec3 shadowRayDirection = neighbourSample.light.position - pixelSample.hitPosition;
	float shadowRayDistance = glm::length(shadowRayDirection);
	shadowRayDirection = glm::normalize(shadowRayDirection);
	glm::vec3 shadowRayOrigin = pixelSample.hitPosition + m_Settings.Eta * shadowRayDirection;
	bool notOccluded = !m_Scene.tlas.IsOccluded(Ray(shadowRayOrigin, shadowRayDirection, shadowRayDistance - 2 * m_Settings.Eta));

	Resevoir spatialResevoir;
	if (withinMaxDistance && sameNormals && notOccluded && neighbourResevoir.WeightSampleOut > 0.01f)
	{
		spatialResevoir = Resevoir::CombineBiased(pixelResevoir, neighbourResevoir, seed);
		pixelSample.ReplaceLight(spatialResevoir.GetSample().light);
		spatialResevoir.SetSample(pixelSample);
		pixelResevoir = spatialResevoir;
	}
}

void Renderer::SpatialReuse(const glm::i32vec2& pixel, const glm::i32vec2& resolution, uint32_t bufferIndex, uint32_t& seed)
{
	m_ResevoirBuffers.GetSpatialReuseBuffer()[bufferIndex] = m_ResevoirBuffers.GetCurrentBuffer()[bufferIndex];

	CombineNeighbourPixel(m_ResevoirBuffers.GetSpatialReuseBuffer()[bufferIndex], pixel, resolution, seed);
	for (int i = 1; i < m_Settings.SpatialReuseNeighbours; i++)
	{
		CombineNeighbourPixel(m_ResevoirBuffers.GetSpatialReuseBuffer()[bufferIndex], pixel, resolution, seed);
	}
}

glm::vec4 Renderer::RenderSample(uint32_t bufferIndex, uint32_t& seed)
{
	// Direct lighting calculation
	glm::vec3 outputColor(0.0f);
	Resevoir resevoir = m_ResevoirBuffers.GetCurrentBuffer()[bufferIndex];
	Sample sample = resevoir.GetSample();

	if (sample.BRDF > 0.001f)
	{
		glm::vec3 shadowRayOrigin = sample.hitPosition + (m_Settings.Eta * sample.lightDirection);
		Ray shadowRay = Ray(shadowRayOrigin, sample.lightDirection, sample.lightDistance - 2.0f * m_Settings.Eta);

		if (!m_Scene.tlas.IsOccluded(shadowRay))
		{
			outputColor = sample.BRDF * sample.light.emmission / (sample.lightDistance * sample.lightDistance);
		}
	}

	return glm::vec4(outputColor * resevoir.WeightSampleOut, 1.0f);
}

// ================= Render Loop =================

void Renderer::RenderFrameBuffer()
{
	while (!m_Terminate)
	{
		auto timeStart = std::chrono::system_clock::now();

		m_FrameBufferLock.lock();
		FrameBufferRef framebuffer = m_FrameBuffers.GetRenderBuffer();
		m_FrameBufferLock.unlock();

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
			m_PrevCamera = m_Scene.camera;
			m_Scene = m_NewScene;
			m_SceneLock.unlock();

			m_Scene.camera.SetResolution(m_Settings.FrameWidth, m_Settings.FrameHeight);
			m_Scene.camera.UpdateState();

			m_Scene.tlas.UpdateTransform();
			m_Scene.tlas.Build();
		}

		uint32_t width = m_Settings.FrameWidth;
		uint32_t height = m_Settings.FrameHeight;

		uint32_t bufferSize = width * height;
		UpdateSampleBufferSize(bufferSize);
		m_FrameBuffers.ResizeRenderBuffer(bufferSize);
		m_ResevoirBuffers.ResizeBuffers(bufferSize);

		if (m_Settings.Mode != RendererSettings::RenderMode::ReSTIR)
		{
			TaskBatch taskBatch(m_Settings.ThreadCount);
			for (uint32_t y = 0; y < height; y += m_Settings.TileSize)
			{
				for (uint32_t x = 0; x < width; x += m_Settings.TileSize)
				{
					uint32_t seed = x + y * width;
					taskBatch.EnqueueTask([=]() {RenderKernelNonReSTIR(framebuffer, width, height, x, y, seed); });
				}
			}
			taskBatch.ExecuteTasks();
		}
		else
		{
			auto ReSTIRRender = [&](ReSTIRPass restirPass, TaskBatch& taskBatch) {
				for (uint32_t y = 0; y < height; y += m_Settings.TileSize)
				{
					int yOffset = y * width;
					for (uint32_t x = 0; x < width; x += m_Settings.TileSize)
					{
						taskBatch.EnqueueTask([=]() { RenderKernelReSTIR(framebuffer, width, height, x, y, restirPass, x + yOffset); });
					}
				}
				taskBatch.ExecuteTasks();
			};

			TaskBatch taskBatch(m_Settings.ThreadCount);
			ReSTIRRender(ReSTIRPass::RIS, taskBatch);

			if (m_Settings.EnableVisibilityPass)
				ReSTIRRender(ReSTIRPass::Visibility, taskBatch);

			if (m_Settings.EnableTemporalReuse && m_ValidHistory)
				ReSTIRRender(ReSTIRPass::Temporal, taskBatch);

			if (m_Settings.EnableSpatialReuse)
			{
				ReSTIRRender(ReSTIRPass::Spatial, taskBatch);
				m_ResevoirBuffers.SwapSpatialBuffers();
			}

			ReSTIRRender(ReSTIRPass::Shading, taskBatch);
		}

		auto timeEnd = std::chrono::system_clock::now();
		m_LastFrameTime = std::chrono::duration<float, std::ratio<1, 1000>>(timeEnd - timeStart).count();

		m_FrameBufferLock.lock();
		m_FrameBuffers.SwapBuffers();
		m_ResevoirBuffers.SwapTemporalBuffers();
		m_FrameBufferLock.unlock();

		m_ValidHistory = true && m_ValidHistoryNextFrame;
		m_ValidHistoryNextFrame = true;
	}
}

void Renderer::RenderKernelNonReSTIR(FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, uint32_t seed)
{
	uint32_t xMax = std::min(xMin + m_Settings.TileSize, width);
	uint32_t yMax = std::min(yMin + m_Settings.TileSize, height);

	if (m_Settings.RandomSeed)
	{
		auto duration = std::chrono::system_clock::now().time_since_epoch();
		auto milliseconds = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
		seed += milliseconds;
	}

	glm::vec4 colorAccumulator;
	auto renderKernel = [&](std::function<glm::vec4(Ray&, const TLAS&, const RendererSettings&, uint32_t&)> renderFunction)
	{
		for (uint32_t y = yMin; y < yMax; y++)
		{
			for (uint32_t x = xMin; x < xMax; x++)
			{
				Ray ray = m_Scene.camera.GetRay(x, y);
				Utils::FillFrameBufferPixel(x, y, renderFunction(ray, m_Scene.tlas, m_Settings, seed), width, frameBuffer);
			}
		}
	};

	switch (m_Settings.Mode)
	{
	case RendererSettings::RenderMode::Normals:
		renderKernel(RenderModeNormal::RenderRay);
		break;
	case RendererSettings::RenderMode::TraversalSteps:
		renderKernel(RenderModeTraversalSteps::RenderRay);
		break;
	case RendererSettings::RenderMode::DI:
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
	uint32_t xMax = std::min(xMin + m_Settings.TileSize, width);
	uint32_t yMax = std::min(yMin + m_Settings.TileSize, height);

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
				VisibilityPass(x + yOffset);
			}
		}
		break;
	case ReSTIRPass::Temporal:
		for (uint32_t y = yMin; y < yMax; y++)
		{
			uint32_t yOffset = y * width;
			for (uint32_t x = xMin; x < xMax; x++)
			{
				TemporalReuse(glm::i32vec2(x, y), glm::i32vec2(width, height), x + yOffset, seed);
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
				Utils::FillFrameBufferPixel(x, y, RenderSample(x + yOffset, seed), width, frameBuffer);
			}
		}
		break;
	}
}