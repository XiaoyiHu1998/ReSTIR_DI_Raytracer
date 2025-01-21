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
		m_SampleOut{ T()}, m_SampleCount{ 0 }, m_WeightTotal{ 0 }
	{}

	Resevoir(T initialSample, float weight) :
		m_SampleOut{ initialSample }, m_SampleCount{ 1 }, m_WeightTotal{ weight }
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

	T GetSampleOut() const { return m_SampleOut; }
	float GetWeightTotal() const { return m_WeightTotal; }
	int GetSampleCount() const { return m_SampleCount; }
};

class Renderer
{
public:
	struct Settings
	{
		enum class RenderMode
		{
			Normals = 0,
			TraversalSteps = 1,
			DI = 2,
			ReSTIR = 3
		};

		RenderMode Mode = RenderMode::DI;

		uint32_t ThreadCount = std::thread::hardware_concurrency() - 2;

		uint32_t RenderResolutionWidth = 640;
		uint32_t RenderResolutionHeight = 480;
		uint32_t RenderingKernelSize = 16;
		uint32_t SamplesPerPixel = 1;
		uint32_t MaxRayDepth = 1;

		bool RandomSeed = true;
		float Eta = 0.001f;

		// DI Rendering
		bool LightOcclusionCheckDI = true;
		bool SampleAllLightsDI = false;

		int CandidateCountDI = 1;

		// ReSTIR Rendering
		bool LightOcclusionCheckCandidatesReSTIR = false;
		bool LightOcclusionCheckShadingReSTIR = true;
		bool SpatialReuse = true;

		int CandidateCountReSTIR = 1;
		int SpatialReuseNeighbours = 3;
		int SpatialReuseRadius = 15;
		float SpatialReuseMaxDistance = 0.1f;
		float SpatialReuseMinNormalSimilarity = 0.85f;
		int SpatialReuseIterationCount = 1;
	};
private:
	Settings m_Settings;
	float m_LastFrameTime;

	std::vector<Sample> m_SampleBuffer;
	std::vector<Resevoir<Sample>> m_ResevoirBuffer;

	glm::vec3 Reflect(const glm::vec3& incomingDirection, const glm::vec3& normal);
	glm::vec3 Refract(const glm::vec3& incomingDirection, const glm::vec3& normal, const float eta_t, const float eta_i = 1.f);
	glm::vec3 RandomPointOnHemisphere(const glm::vec3& normal, uint32_t& seed);
	glm::vec3 CosineSampleHemisphere(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& tangent, uint32_t& seed);
	LightSampleInfo SampleRandomLight(const glm::vec3& hitLocation, const std::vector<Sphere>& sphereLights, uint32_t& seed);
	LightSampleInfo SampleRandomLight(const glm::vec3& hitLocation, const TLAS& tlasEmmisive, uint32_t& seed);
private:
	void Renderer::RenderKernelFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const TLAS& tlasEmmisive, const std::vector<Sphere>& sphereLights, uint32_t seed);
	glm::vec4 RenderDI(Ray& ray, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed);
	
	// A GentleIntroduction To ReSTIR
	Sample SamplePointLight(const Camera& camera, const glm::i32vec2& pixel, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed);
	void GenerateSample(const Camera& camera, const glm::i32vec2 pixel, uint32_t bufferIndex, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed);
	glm::vec3 TargetDistribution(const PathDI& path);
	Sample ShiftSampleSpatially(const Sample& sample, const Sample& neighbourSample, const TLAS& tlas);
	void SpatialReuse(const glm::i32vec2& pixel, const glm::i32vec2& resolution, const TLAS& tlas, uint32_t& seed);
	void VisibilityPass(Sample& sample);
	glm::vec4 RenderSample(Sample sample, const TLAS& tlas, uint32_t& seed);

	// ReSTIR original paper
	Resevoir<Sample> GenerateSamplePaper(const Camera& camera, const glm::i32vec2 pixel, uint32_t bufferIndex, const TLAS& tlas, const std::vector<Sphere>& sphereLights, uint32_t& seed);
	void VisibilityPassPaper(Resevoir<Sample>& resevoir, const TLAS& tlas);
	Resevoir<Sample> SpatialReusePaper(const glm::i32vec2& pixel, const glm::i32vec2& resolution, uint32_t bufferIndex, uint32_t& seed);
	glm::vec4 RenderSamplePaper(const Resevoir<Sample>& resevoir, const TLAS& tlas, uint32_t& seed);
public:
	Renderer() :
		m_LastFrameTime{ 0.0f }, m_SampleBuffer{ std::vector<Sample>() }
	{
		m_SampleBuffer.reserve(m_Settings.RenderResolutionWidth * m_Settings.RenderResolutionHeight);
	}

	void RenderFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, const TLAS& tlas, const TLAS& tlasEmmisive, const std::vector<Sphere>& sphereLights);

	Settings& GetSettings()  { return m_Settings; }
	float GetLastFrameTime() { return m_LastFrameTime; }

	void UpdateSampleBufferSize(uint32_t bufferSize)
	{
		m_SampleBuffer.resize(bufferSize);
	}
};