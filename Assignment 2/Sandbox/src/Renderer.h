#pragma once

#include <thread>
#include <mutex>

#include "Glad/include/glad/glad.h"

#include "Include.h"
#include "Camera.h"
#include "Ray.h"
#include "Primitives.h"
#include "AccelerationStructures.h"

struct PathDI
{
	glm::vec3 CameraOrigin;
	glm::vec3 HitLocation;
	glm::vec3 LightLocation;

	HitInfo FirstRayHitInfo;
	HitInfo ShadowRayHitInfo;
	Sphere Light;

	PathDI() = default;

	PathDI(const glm::vec3 cameraOrigin, const glm::vec3& hitLocation, const glm::vec3& lightLocation) :
		CameraOrigin{ cameraOrigin }, HitLocation{ hitLocation }, LightLocation{ lightLocation },
		FirstRayHitInfo{ HitInfo() }, ShadowRayHitInfo{ HitInfo() }, Light{ Sphere() }
	{}

	PathDI(const glm::vec3 cameraOrigin, const glm::vec3 & hitLocation, const glm::vec3 & lightLocation,
		const HitInfo & firstRayHitInfo, const HitInfo & ShadowRayHitInfo, const Sphere & light) :
		CameraOrigin{ cameraOrigin }, HitLocation{ hitLocation }, LightLocation{ lightLocation },
		FirstRayHitInfo{ firstRayHitInfo }, ShadowRayHitInfo{ ShadowRayHitInfo }, Light{ light }
	{}
};

struct Sample
{
	bool valid;

	PathDI Path;
	float Weight;
	float PDF;

	Sample() :
		valid{ false }
	{}

	Sample(const glm::vec3 cameraOrigin, const glm::vec3 & hitLocation, const glm::vec3 & lightLocation, float weight) :
		valid{ false }, Path{ cameraOrigin, hitLocation, lightLocation}, Weight{weight}
	{}

	Sample(Sample sample, float weight) :
		valid{ sample.valid }, Path{ sample.Path }, Weight{ weight }
	{}
};

template <class T>
class Resevoir
{
public:
	float WeightSampleOut;
private:
	T m_SampleOut;
	float m_WeightTotal;

	int m_SampleCount;
public:
	Resevoir() :
		m_SampleOut{ T() }, m_SampleCount{ 0 }, m_WeightTotal{ 0.0f }, WeightSampleOut{ 0.0f }
	{}

	Resevoir(T initialSample, float totalWeight, uint32_t totalSampleCount) :
		m_SampleOut{ initialSample }, m_SampleCount{ 1 }, m_WeightTotal{ totalWeight }, m_SampleCount{ totalSampleCount }
	{}

	bool Update(const T& sample, float weight, uint32_t& seed)
	{
		m_SampleCount++;
		m_WeightTotal += weight;

		
		if (Utils::RandomFloat(seed) < weight / m_WeightTotal)
		{
			m_SampleOut = sample;
			return true;
		}

		return false;
	}

	void SetSampleCount(uint32_t sampleCount) { m_SampleCount = sampleCount; }

	T GetSampleOut() const { return m_SampleOut; }
	T& GetSampleOutRef() { return m_SampleOut; }
	float GetWeightTotal() const { return m_WeightTotal; }
	int GetSampleCount() const { return m_SampleCount; }
};

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
		int CandidateCountReSTIR = 4;

		bool EnableVisibilityPass = true;
		bool EnableSpatialReuse = true;

		int SpatialReuseNeighbours = 4;
		int SpatialReuseRadius = 10;
		float SpatialReuseMaxDistance = 0.06f;
		float SpatialReuseMinNormalSimilarity = 0.90f;

		bool EnableTemporalReuse = true;
	};

	struct Scene
	{
		Camera camera;
		TLAS tlas;
		std::vector<Sphere> sphereLights;

		Scene() = default;

		Scene(Camera& camera, TLAS& tlas, std::vector<Sphere>& sphereLights) :
			camera{ camera }, tlas{ tlas }, sphereLights{ sphereLights }
		{}
	};
private:
	FrameDoubleBuffer m_FrameBuffers;
	Settings m_Settings;
	Scene m_Scene;

	std::thread m_RenderThread;
	std::mutex m_FrameBufferMutex;
	std::mutex m_SettingsLock;
	std::mutex m_SceneLock;

	float m_LastFrameTime;

	std::vector<Sample> m_SampleBuffer;
	std::vector<Resevoir<Sample>> m_ResevoirBuffers[2];
	int m_CurrentBuffer;
	int m_PrevBuffer;
private:
	void RenderFrameBuffer();
	void RenderFrameBufferTest();

	void Renderer::RenderThreadNonReSTIR(uint32_t startY, uint32_t endY, uint32_t endX, const Camera& camera, FrameBufferRef frameBuffer, const TLAS& tlas, const std::vector<Sphere>& sphereLights, const Settings& settings, uint32_t seed);

	void Renderer::RenderKernelNonReSTIR(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t seed);
	void Renderer::RenderKernelReSTIR(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const std::vector<Sphere>& sphereLights, ReSTIRPass restirPass, uint32_t seed);
	glm::vec4 RenderDI(Ray& ray, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed);

	// ReSTIR original paper
	Sample SamplePointLight(const Camera& camera, const glm::i32vec2& pixel, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed);
	glm::vec3 TargetDistribution(const PathDI& path);
	Resevoir<Sample> CombineResevoirBiased(const Resevoir<Sample>& originalResevoir, Resevoir<Sample>& newResevoir, uint32_t& seed);
	Resevoir<Sample> CombineResevoirsBiased(const Resevoir<Sample>& originalResevoir, std::vector<Resevoir<Sample>>& newResevoirs, uint32_t& seed);
	//Resevoir<Sample> CombineResevoirsUnbiased(const Resevoir<Sample>& originalResevoir, std::vector<Resevoir<Sample>>& newResevoirs, uint32_t& seed);

	Resevoir<Sample> GenerateSample(const Camera& camera, const glm::i32vec2 pixel, uint32_t bufferIndex, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed);
	inline void VisibilityPass(Resevoir<Sample>& resevoir, const TLAS& tlas);
	inline void SpatialReuse(const glm::i32vec2& pixel, const glm::i32vec2& resolution, uint32_t bufferIndex, uint32_t& seed);
	inline glm::i32vec2 GetTemporalNeighbourPixel(const Camera& camera, const glm::i32vec2& resolution, const glm::vec3& hitLocation);
	inline void TemporalReuse(const Camera& camera, const Resevoir<Sample>& resevoir, const glm::i32vec2& pixel, const glm::i32vec2 resolution, uint32_t bufferIndex, uint32_t& seed);
	inline glm::vec4 RenderSample(const Resevoir<Sample>& resevoir, const TLAS& tlas, uint32_t& seed);
public:
	Renderer() :
		m_LastFrameTime{ 0.0f }, m_SampleBuffer{ std::vector<Sample>() }
	{
		//m_SampleBuffer.reserve(m_Settings.RenderResolutionWidth* m_Settings.RenderResolutionHeight);
		m_ResevoirBuffers[0] = std::vector<Resevoir<Sample>>();
		m_ResevoirBuffers[1] = std::vector<Resevoir<Sample>>();
		//m_ResevoirBuffers[0].reserve(m_Settings.RenderResolutionWidth* m_Settings.RenderResolutionHeight);
		//m_ResevoirBuffers[1].reserve(m_Settings.RenderResolutionWidth* m_Settings.RenderResolutionHeight);

		m_CurrentBuffer = 0;
		m_PrevBuffer = 1;

		m_FrameBuffers = FrameDoubleBuffer();
	}

	void Init(const Scene& scene)
	{
		m_Scene = scene; // Doesn't need to lock due to render thread not being spawned yet.
		m_RenderThread = std::thread(&Renderer::RenderFrameBuffer, this);
	}

	void SubmitRenderSettings(const Settings& newRenderSettings) 
	{ 
		m_SettingsLock.lock();
		m_Settings = newRenderSettings;
		m_SettingsLock.unlock();
	}

	void SubmitScene(const Scene& newScene) 
	{
		m_SceneLock.lock();
		m_Scene = newScene;
		m_SceneLock.unlock();
	}

	void UpdateSampleBufferSize(uint32_t bufferSize)
	{
		if (m_SampleBuffer.size() != bufferSize)
		{
			m_SampleBuffer.resize(bufferSize);
		}
	}

	//TODO: Need to make resize happen after flipping m_CurrentBuffer to support resizing during temporal reuse
	void UpdateResevoirBufferSize(uint32_t bufferSize)
	{
		if (m_ResevoirBuffers[0].size() != bufferSize || m_ResevoirBuffers[1].size() != bufferSize)
		{
			m_ResevoirBuffers[0].resize(bufferSize);
			m_ResevoirBuffers[1].resize(bufferSize);
		}
	}

	//Settings& GetSettings() { return m_Settings; }
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