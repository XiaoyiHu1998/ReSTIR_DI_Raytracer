#pragma once

#include <thread>

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
		uint32_t RenderingKernelSize = 16;
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
		bool EnableSpatialReuse = false;

		int SpatialReuseIterationCount = 1;
		int SpatialReuseNeighbours = 4;
		int SpatialReuseRadius = 10;
		float SpatialReuseMaxDistance = 0.06f;
		float SpatialReuseMinNormalSimilarity = 0.90f;

		bool EnableTemporalReuse = false;

	};
private:
	Settings m_Settings;
	float m_LastFrameTime;

	std::vector<Sample> m_SampleBuffer;
	std::vector<Resevoir<Sample>> m_ResevoirBuffers[2];
	int m_CurrentBuffer;
	int m_PrevBuffer;
private:
	void Renderer::RenderKernelNonReSTIR(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t seed);
	void Renderer::RenderKernelReSTIR(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const std::vector<Sphere>& sphereLights, ReSTIRPass restirPass, uint32_t seed);
	glm::vec4 RenderDI(Ray& ray, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed);
	Sample ShiftSampleSpatially(const Resevoir<Sample>& pixelResevoir, const Resevoir<Sample>& neighbourResevoir);

	// ReSTIR original paper
	Sample SamplePointLight(const Camera& camera, const glm::i32vec2& pixel, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed);
	glm::vec3 TargetDistribution(const PathDI& path);
	Resevoir<Sample> CombineResevoirsBiased(const Resevoir<Sample>& originalResevoir, std::vector<Resevoir<Sample>>& newResevoirs, uint32_t& seed);
	//Resevoir<Sample> CombineResevoirsUnbiased(const Resevoir<Sample>& originalResevoir, std::vector<Resevoir<Sample>>& newResevoirs, uint32_t& seed);
	Resevoir<Sample> GenerateSample(const Camera& camera, const glm::i32vec2 pixel, uint32_t bufferIndex, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed);
	void VisibilityPass(Resevoir<Sample>& resevoir, const TLAS& tlas);
	Resevoir<Sample> SpatialReuse(const glm::i32vec2& pixel, const glm::i32vec2& resolution, uint32_t bufferIndex, uint32_t& seed);
	glm::i32vec2 GetTemporalNeighbourPixel(const Camera& camera, const glm::i32vec2& resolution, const glm::vec3& hitLocation);
	void TemporalReuse(const Camera& camera, const Resevoir<Sample>& resevoir, const glm::i32vec2& pixel, const glm::i32vec2 resolution, uint32_t bufferIndex, uint32_t& seed);
	glm::vec4 RenderSample(const Resevoir<Sample>& resevoir, const TLAS& tlas, uint32_t& seed);
public:
	Renderer() :
		m_LastFrameTime{ 0.0f }, m_SampleBuffer{ std::vector<Sample>() }
	{
		m_SampleBuffer.reserve(m_Settings.RenderResolutionWidth* m_Settings.RenderResolutionHeight);
		m_ResevoirBuffers[0] = std::vector<Resevoir<Sample>>();
		m_ResevoirBuffers[1] = std::vector<Resevoir<Sample>>();
		m_ResevoirBuffers[0].reserve(m_Settings.RenderResolutionWidth* m_Settings.RenderResolutionHeight);
		m_ResevoirBuffers[1].reserve(m_Settings.RenderResolutionWidth* m_Settings.RenderResolutionHeight);

		m_CurrentBuffer = 0;
		m_PrevBuffer = 1;
	}

	void RenderFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, const TLAS& tlas, const std::vector<Sphere>& sphereLights);

	Settings& GetSettings()  { return m_Settings; }
	float GetLastFrameTime() { return m_LastFrameTime; }

	void UpdateSampleBufferSize(uint32_t bufferSize)
	{
		m_SampleBuffer.resize(bufferSize);
	}

	//TODO: Need to make resize happen after flipping m_CurrentBuffer to support resizing during temporal reuse
	void UpdateResevoirBufferSize(uint32_t bufferSize)
	{
		m_ResevoirBuffers[0].resize(bufferSize);
		m_ResevoirBuffers[1].resize(bufferSize);
	}
};