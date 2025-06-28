#pragma once

#include <thread>
#include <mutex>

#include "Glad/include/glad/glad.h"

#include "Include.h"
#include "Camera.h"
#include "Ray.h"
#include "Primitives.h"
#include "AccelerationStructures.h"
#include "Utils.h"
#include "ReSTIR.h"

class FrameDoubleBuffer
{
public:
	FrameDoubleBuffer()
	{
		m_NextBuffer = 1;
		m_CurrentBuffer = 0;
		m_FrameBuffers[0] = std::make_shared<std::vector<uint8_t>>();
		m_FrameBuffers[1] = std::make_shared<std::vector<uint8_t>>();
	}

	FrameBufferRef GetFrameBuffer() { return m_FrameBuffers[m_CurrentBuffer]; }
	FrameBufferRef GetRenderBuffer() { return m_FrameBuffers[m_NextBuffer]; }

	void SwapFrameBuffers()
	{
		m_NextBuffer = (m_NextBuffer + 1) % 2;
		m_CurrentBuffer = (m_CurrentBuffer + 1) % 2;
	}
private:
	FrameBufferRef m_FrameBuffers[2];
	uint32_t m_CurrentBuffer;
	uint32_t m_NextBuffer;
};

class Renderer
{
public:
	enum class ReSTIRPass
	{
		RIS,
		Visibility,
		Temporal,
		Spatial,
		Shading
	};

	struct Settings
	{
		enum class RenderMode
		{
			Normals = 0,
			TraversalSteps = 1,
			DI = 2,
			ReSTIR = 3
		};

		RenderMode Mode = RenderMode::ReSTIR;

		uint32_t ThreadCount = std::thread::hardware_concurrency() - 1;

		uint32_t RenderResolutionWidth = 640;
		uint32_t RenderResolutionHeight = 480;
		uint32_t RenderingKernelSize = 64;
		uint32_t SamplesPerPixel = 1;

		bool RandomSeed = true;
		float Eta = 0.001f;

		// DI Rendering
		bool LightOcclusionCheckDI = true;
		bool SampleAllLightsDI = false;

		int CandidateCountDI = 1;

		// ReSTIR Rendering
		int CandidateCountReSTIR = 1;

		bool EnableVisibilityPass = true;

		bool EnableSpatialReuse = true;
		int SpatialReuseNeighbours = 1;
		int SpatialReuseRadius = 10;
		float SpatialReuseMaxDistance = 0.06f;
		float SpatialReuseMinNormalSimilarity = 0.90f;

		bool EnableTemporalReuse = true;
		float TemporalReuseMaxDistance = 0.04f;
		float TemporalReuseMinNormalSimilarity = 0.90f;

		bool operator==(const Settings& otherSettings)
		{
			bool sameSettings = true;
			sameSettings &= Mode == otherSettings.Mode;

			sameSettings &= ThreadCount == otherSettings.ThreadCount;

			sameSettings &= RenderResolutionWidth == otherSettings.RenderResolutionWidth;
			sameSettings &= RenderResolutionHeight == otherSettings.RenderResolutionHeight;
			sameSettings &= RenderingKernelSize == otherSettings.RenderingKernelSize;
			sameSettings &= SamplesPerPixel == otherSettings.SamplesPerPixel;

			sameSettings &= RandomSeed == otherSettings.RandomSeed;
			sameSettings &= Eta == otherSettings.Eta;

			// DI Rendering
			sameSettings &= LightOcclusionCheckDI == otherSettings.LightOcclusionCheckDI;
			sameSettings &= SampleAllLightsDI == otherSettings.SampleAllLightsDI;

			sameSettings &= CandidateCountDI == otherSettings.CandidateCountDI;

			// ReSTIR Rendering
			sameSettings &= CandidateCountReSTIR == otherSettings.CandidateCountReSTIR;

			sameSettings &= EnableVisibilityPass == otherSettings.EnableVisibilityPass;

			sameSettings &= EnableSpatialReuse == otherSettings.EnableSpatialReuse;
			sameSettings &= SpatialReuseNeighbours == otherSettings.SpatialReuseNeighbours;
			sameSettings &= SpatialReuseRadius == otherSettings.SpatialReuseRadius;
			sameSettings &= SpatialReuseMaxDistance == otherSettings.SpatialReuseMaxDistance;
			sameSettings &= SpatialReuseMinNormalSimilarity == otherSettings.SpatialReuseMinNormalSimilarity;

			sameSettings &= EnableTemporalReuse == otherSettings.EnableTemporalReuse;
			sameSettings &= TemporalReuseMaxDistance == otherSettings.TemporalReuseMaxDistance;
			sameSettings &= TemporalReuseMinNormalSimilarity == otherSettings.TemporalReuseMinNormalSimilarity;

			return sameSettings;
		}

		bool operator!=(const Settings& otherSettings)
		{
			return !operator==(otherSettings);
		}
	};

	struct Scene
	{
		Camera camera;
		TLAS tlas;
		std::vector<PointLight> pointLights;

		Scene() = default;

		Scene(const Camera& camera, const TLAS& tlas, const std::vector<PointLight>& pointLights) :
			camera{ camera }, tlas{ tlas }, pointLights{ pointLights }
		{}
	};
private:
	FrameDoubleBuffer m_FrameBuffers;
	Settings m_Settings;
	Scene m_Scene;

	Settings m_NewSettings;
	Scene m_NewScene;

	std::thread m_RenderThread;
	std::mutex m_FrameBufferMutex;
	std::mutex m_ResevoirBufferMutex;
	std::mutex m_SettingsLock;
	std::mutex m_SceneLock;

	bool SettingsUpdated;
	bool SceneUpdated;

	float m_LastFrameTime;

	std::vector<Sample> m_SampleBuffer;
	std::vector<Resevoir> m_ResevoirBuffers[2];
	int m_CurrentBuffer;
	int m_PrevBuffer;
	bool m_ValidHistory;
private:
	void RenderFrameBuffer();
	void Renderer::RenderKernelNonReSTIR(FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, uint32_t seed);
	void Renderer::RenderKernelReSTIR(FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, ReSTIRPass restirPass, uint32_t seed);
	glm::vec4 RenderDI(Ray& ray, uint32_t& seed);

	// ResTIR passes
	inline void GenerateSample(const glm::i32vec2 pixel, uint32_t bufferIndex, uint32_t& seed);
	inline void VisibilityPass(Resevoir& resevoir);
	inline void TemporalReuse(const glm::i32vec2& pixel, const glm::i32vec2 resolution, uint32_t bufferIndex, uint32_t& seed);
	inline void SpatialReuse(const glm::i32vec2& pixel, const glm::i32vec2& resolution, uint32_t bufferIndex, uint32_t& seed);
	inline glm::vec4 RenderSample(const Resevoir& resevoir, uint32_t& seed);
public:
	Renderer() :
		m_LastFrameTime{ 0.0f }, m_SampleBuffer{ std::vector<Sample>() }
	{
		//m_SampleBuffer.reserve(m_Settings.RenderResolutionWidth* m_Settings.RenderResolutionHeight);
		m_ResevoirBuffers[0] = std::vector<Resevoir>();
		m_ResevoirBuffers[1] = std::vector<Resevoir>();
		m_ResevoirBuffers[0].resize(m_Settings.RenderResolutionWidth * m_Settings.RenderResolutionHeight);
		m_ResevoirBuffers[1].resize(m_Settings.RenderResolutionWidth * m_Settings.RenderResolutionHeight);

		m_CurrentBuffer = 0;
		m_PrevBuffer = 1;

		m_FrameBuffers = FrameDoubleBuffer();

		SettingsUpdated = false;
		SceneUpdated = false;
		m_ValidHistory = false;
	}

	void Init(const Scene& scene)
	{
		m_Scene = scene; // Doesn't need to lock due to render thread not being spawned yet.
		m_RenderThread = std::thread(&Renderer::RenderFrameBuffer, this);
	}

	void InvalidateHistory() { m_ValidHistory = false; }

	void SubmitRenderSettings(const Settings& newRenderSettings) 
	{ 
		m_SettingsLock.lock();
		m_NewSettings = newRenderSettings;
		SettingsUpdated = true;
		m_SettingsLock.unlock();
	}

	void SubmitScene(const Scene& newScene) 
	{
		m_SceneLock.lock();
		m_NewScene = newScene;
		SceneUpdated = true;
		m_SceneLock.unlock();
	}

	void UpdateSampleBufferSize(uint32_t bufferSize)
	{
		if (m_SampleBuffer.size() != bufferSize)
		{
			m_SampleBuffer.resize(bufferSize);
		}
	}

	void UpdateResevoirBufferSize(uint32_t bufferSize)
	{
		if (m_ResevoirBuffers[0].size() != bufferSize || m_ResevoirBuffers[1].size() != bufferSize)
		{
			m_ResevoirBuffers[0].resize(bufferSize);
			m_ResevoirBuffers[1].resize(bufferSize);
		}
	}

	float GetLastFrameTime() { return m_LastFrameTime; }

	glm::i32vec2 GetRenderResolution()
	{
		m_SceneLock.lock();
		glm::i32vec2 resolution = m_Scene.camera.GetResolution();
		m_SceneLock.unlock();

		return resolution;
	}

	FrameBufferRef GetFrameBuffer()
	{
		m_FrameBufferMutex.lock();
		FrameBufferRef frameBuffer = m_FrameBuffers.GetFrameBuffer();
		m_FrameBufferMutex.unlock();

		return frameBuffer;
	}
};