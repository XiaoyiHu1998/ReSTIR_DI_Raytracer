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

class DoubleFrameBuffer
{
public:
	DoubleFrameBuffer()
	{
		m_NextBuffer = 1;
		m_CurrentBuffer = 0;
		m_FrameBuffers[0] = std::make_shared<std::vector<uint8_t>>();
		m_FrameBuffers[1] = std::make_shared<std::vector<uint8_t>>();
	}

	void SwapBuffers()
	{
		m_NextBuffer = (m_NextBuffer + 1) % 2;
		m_CurrentBuffer = (m_CurrentBuffer + 1) % 2;
	}

	FrameBufferRef GetFrameBuffer() { return m_FrameBuffers[m_CurrentBuffer]; }
	FrameBufferRef GetRenderBuffer() { return m_FrameBuffers[m_NextBuffer]; }
	void ResizeRenderBuffer(uint32_t bufferSize)
	{
		uint32_t subPixelCount = bufferSize << 2;
		if (m_FrameBuffers[m_NextBuffer]->size() != subPixelCount)
		{
			m_FrameBuffers[m_NextBuffer]->resize(subPixelCount);
		}
	}
private:
	FrameBufferRef m_FrameBuffers[2];
	uint32_t m_CurrentBuffer;
	uint32_t m_NextBuffer;
};

class DoubleResevoirBuffer
{
public:
	DoubleResevoirBuffer()
	{
		m_PrevBuffer = 1;
		m_CurrentBuffer = 0;
		m_ResevoirBuffers[0] = std::vector<Resevoir>();
		m_ResevoirBuffers[1] = std::vector<Resevoir>();
	}

	void SwapBuffers()
	{
		m_PrevBuffer = (m_PrevBuffer + 1) % 2;
		m_CurrentBuffer = (m_CurrentBuffer + 1) % 2;
	}

	std::vector<Resevoir>& GetCurrentBuffer() { return m_ResevoirBuffers[m_CurrentBuffer]; }
	std::vector<Resevoir>& GetPrevBuffer() { return m_ResevoirBuffers[m_PrevBuffer]; }
	void ResizeBuffers(uint32_t bufferSize) 
	{
		if (m_ResevoirBuffers[0].size() != bufferSize || m_ResevoirBuffers[1].size() != bufferSize)
		{
			m_ResevoirBuffers[0].resize(bufferSize); m_ResevoirBuffers[1].resize(bufferSize);
		}
	}
private:
	std::vector<Resevoir> m_ResevoirBuffers[2];
	uint32_t m_CurrentBuffer;
	uint32_t m_PrevBuffer;
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

		uint32_t FrameWidth = 3840;
		uint32_t FrameHeight = 2160;
		uint32_t KernelSize = 32;
		uint32_t SamplesPerPixel = 1;

		bool RandomSeed = true;
		float Eta = 0.001f;

		// DI Rendering
		bool OcclusionCheckDI = true;
		bool SampleAllLightsDI = false;
		int CandidateCountDI = 1;

		// ReSTIR Rendering
		// RIS
		int CandidateCountReSTIR = 3;
		bool EnableVisibilityPass = true;

		// Temporal Reuse
		bool EnableTemporalReuse = true;
		int TemporalSampleCountRatio = 25;
		float TemporalMaxDistance = 0.1f;
		float TemporalMaxDistanceDepthScaling = 0.18f;
		float TemporalMinNormalSimilarity = 0.93f;
		
		// Spatial Reuse
		bool EnableSpatialReuse = true;
		int SpatialReuseNeighbours = 3;
		int SpatialPixelRadius = 15;
		float SpatialMaxDistance = 0.06f;
		float SpatialMaxDistanceDepthScaling = 0.005f;
		float SpatialMinNormalSimilarity = 0.90f;

		bool operator==(const Settings& otherSettings)
		{
			bool sameSettings = true;
			sameSettings &= Mode == otherSettings.Mode;

			sameSettings &= ThreadCount == otherSettings.ThreadCount;

			sameSettings &= FrameWidth == otherSettings.FrameWidth;
			sameSettings &= FrameHeight == otherSettings.FrameHeight;
			sameSettings &= KernelSize == otherSettings.KernelSize;
			sameSettings &= SamplesPerPixel == otherSettings.SamplesPerPixel;

			sameSettings &= RandomSeed == otherSettings.RandomSeed;
			sameSettings &= Eta == otherSettings.Eta;

			// DI Rendering
			sameSettings &= OcclusionCheckDI == otherSettings.OcclusionCheckDI;
			sameSettings &= SampleAllLightsDI == otherSettings.SampleAllLightsDI;

			sameSettings &= CandidateCountDI == otherSettings.CandidateCountDI;

			// ReSTIR RIS
			sameSettings &= CandidateCountReSTIR == otherSettings.CandidateCountReSTIR;
			sameSettings &= EnableVisibilityPass == otherSettings.EnableVisibilityPass;

			// ReSTIR Temporal Reuse
			sameSettings &= EnableTemporalReuse == otherSettings.EnableTemporalReuse;
			sameSettings &= TemporalMaxDistance == otherSettings.TemporalMaxDistance;
			sameSettings &= TemporalMaxDistanceDepthScaling == otherSettings.TemporalMaxDistanceDepthScaling;
			sameSettings &= TemporalMinNormalSimilarity == otherSettings.TemporalMinNormalSimilarity;
			sameSettings &= TemporalSampleCountRatio == otherSettings.TemporalSampleCountRatio;

			// ReSTIR Spatial Reuse
			sameSettings &= EnableSpatialReuse == otherSettings.EnableSpatialReuse;
			sameSettings &= SpatialReuseNeighbours == otherSettings.SpatialReuseNeighbours;
			sameSettings &= SpatialPixelRadius == otherSettings.SpatialPixelRadius;
			sameSettings &= SpatialMaxDistance == otherSettings.SpatialMaxDistance;
			sameSettings &= SpatialMaxDistanceDepthScaling == otherSettings.SpatialMaxDistanceDepthScaling;
			sameSettings &= SpatialMinNormalSimilarity == otherSettings.SpatialMinNormalSimilarity;

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
	std::vector<Sample> m_SampleBuffer;
	DoubleFrameBuffer m_FrameBuffers;
	DoubleResevoirBuffer m_ResevoirBuffers;
	bool m_ValidHistory;

	Settings m_Settings;
	Scene m_Scene;
	Camera m_PrevCamera;

	Settings m_NewSettings;
	Scene m_NewScene;

	std::thread m_RenderThread;
	std::mutex m_FrameBufferMutex;
	std::mutex m_SettingsLock;
	std::mutex m_SceneLock;

	bool SettingsUpdated;
	bool SceneUpdated;

	float m_LastFrameTime;

private:
	void RenderFrameBuffer();
	void Renderer::RenderKernelNonReSTIR(FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, uint32_t seed);
	void Renderer::RenderKernelReSTIR(FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, ReSTIRPass restirPass, uint32_t seed);
	glm::vec4 RenderDI(Ray& ray, uint32_t& seed);

	// ResTIR passes
	inline void GenerateSample(const glm::i32vec2 pixel, uint32_t bufferIndex, uint32_t& seed);
	inline void VisibilityPass(uint32_t bufferIndex);
	inline void TemporalReuse(const glm::i32vec2& pixel, const glm::i32vec2 resolution, uint32_t bufferIndex, uint32_t& seed);
	inline void SpatialReuse(const glm::i32vec2& pixel, const glm::i32vec2& resolution, uint32_t bufferIndex, uint32_t& seed);
	inline glm::vec4 RenderSample(uint32_t bufferIndex, uint32_t& seed);
public:
	Renderer() :
		m_LastFrameTime{ 0.0f }, m_SampleBuffer{ std::vector<Sample>() }
	{
		m_FrameBuffers = DoubleFrameBuffer();
		m_ResevoirBuffers = DoubleResevoirBuffer();

		SettingsUpdated = false;
		SceneUpdated = false;
		m_ValidHistory = false;
	}

	void Init(const Settings& settings, const Scene& scene)
	{
		m_Settings = settings;
		m_Scene = scene; // Doesn't need to lock due to render thread not being spawned yet.
		m_PrevCamera = scene.camera;
		m_FrameBuffers.ResizeRenderBuffer(m_Settings.FrameWidth * m_Settings.FrameHeight);
		m_FrameBuffers.SwapBuffers();
		m_FrameBuffers.ResizeRenderBuffer(m_Settings.FrameWidth * m_Settings.FrameHeight);

		// Start rendering
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
		if (m_ResevoirBuffers.GetCurrentBuffer().size() != bufferSize || m_ResevoirBuffers.GetPrevBuffer().size() != bufferSize)
		{
			m_ResevoirBuffers.GetCurrentBuffer().resize(bufferSize);
			m_ResevoirBuffers.GetPrevBuffer().resize(bufferSize);
		}
	}

	float GetLastFrameTime() { return m_LastFrameTime; }

	glm::i32vec2 GetRenderResolution()
	{
		m_SettingsLock.lock();
		glm::i32vec2 resolution = glm::i32vec2(m_Settings.FrameWidth, m_Settings.FrameHeight);
		m_SettingsLock.unlock();

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