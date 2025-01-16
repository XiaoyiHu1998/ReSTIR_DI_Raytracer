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
	LightSampleInfo LightSample;

	PathDI() = default;

	PathDI(const glm::vec3 cameraOrigin, const glm::vec3& hitLocation, const glm::vec3& lightLocation) :
		CameraOrigin{ cameraOrigin }, HitLocation{ hitLocation }, LightLocation{ lightLocation },
		FirstRayHitInfo{ HitInfo() }, ShadowRayHitInfo{ HitInfo() }, LightSample{ LightSampleInfo() }
	{}

	PathDI(const glm::vec3 cameraOrigin, const glm::vec3 & hitLocation, const glm::vec3 & lightLocation,
		const HitInfo & firstRayHitInfo, const HitInfo & ShadowRayHitInfo, const LightSampleInfo & LightSample) :
		CameraOrigin{ cameraOrigin }, HitLocation{ hitLocation }, LightLocation{ lightLocation },
		FirstRayHitInfo{ firstRayHitInfo }, ShadowRayHitInfo{ ShadowRayHitInfo }, LightSample{ LightSample }
	{}
};

struct Sample
{
	bool valid;

	PathDI Path;
	float Weight;

	Sample() :
		valid{ false }
	{}

	Sample(const glm::vec3 cameraOrigin, const glm::vec3 & hitLocation, const glm::vec3 & lightLocation, float weight) :
		valid{ false }, Path{ cameraOrigin, hitLocation, lightLocation}, Weight{weight}
	{}

	Sample(Sample sample, float weight) :
		valid{ false }, Path{ sample.Path }, Weight{ weight }
	{}
};

template <class T>
class Resevoir
{
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

	T GetSample() { return m_SampleOut; }
	float GetWeightTotal() { return m_WeightTotal; }
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
			Pathtrace = 2,
			ReSTIR = 3
		};

		RenderMode Mode = RenderMode::ReSTIR;

		uint32_t ThreadCount = std::thread::hardware_concurrency() - 2;

		uint32_t RenderResolutionWidth = 640;
		uint32_t RenderResolutionHeight = 480;
		uint32_t RenderingKernelSize = 16;
		uint32_t SamplesPerPixel = 1;
		uint32_t MaxRayDepth = 8;

		// ReSTIR
		uint32_t CanidateCountReSTIR = 4;
	};
private:
	Settings m_Settings;
	float m_LastFrameTime;

	std::vector<Sample> m_SampleBuffer;

	glm::vec3 Reflect(const glm::vec3& incomingDirection, const glm::vec3& normal);
	glm::vec3 Refract(const glm::vec3& incomingDirection, const glm::vec3& normal, const float eta_t, const float eta_i = 1.f);
	glm::vec3 RandomPointOnHemisphere(const glm::vec3& normal, uint32_t& seed);
	glm::vec3 CosineSampleHemisphere(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& tangent, uint32_t& seed);

	void Renderer::RenderKernelFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, uint32_t xMin, uint32_t yMin, const TLAS& tlas, const TLAS& tlasEmmisive, uint32_t seed);
private:
	Sample SampleAreaLights(const Camera& camera, const glm::i32vec2& pixel, const TLAS& tlas, const TLAS& tlasEmmisive, uint32_t& seed);
	void GenerateSample(const Camera& camera, const glm::i32vec2 pixel, uint32_t bufferIndex, const TLAS& tlas, const TLAS& tlasEmmisive, uint32_t& seed);
	glm::vec3 TargetDistribution(const PathDI& path);
	glm::vec4 RenderSample(Sample sample, const TLAS& tlas, const TLAS& tlasEmmisive, uint32_t& seed);
public:
	Renderer() :
		m_LastFrameTime{ 0.0f }, m_SampleBuffer{ std::vector<Sample>() }
	{
		m_SampleBuffer.reserve(m_Settings.RenderResolutionWidth * m_Settings.RenderResolutionHeight);
	}

	glm::vec4 PathTraceRay(Ray& ray, const TLAS& tlas, const TLAS& tlasEmmisive, uint32_t& seed);
	void RenderFrameBuffer(Camera camera, FrameBufferRef frameBuffer, uint32_t width, uint32_t height, const TLAS& tlas, const TLAS& tlasEmmisive);
	LightSampleInfo SampleRandomLight(const Ray& ray, const TLAS& tlasEmmisive, uint32_t& seed);

	Settings& GetSettings()  { return m_Settings; }
	float GetLastFrameTime() { return m_LastFrameTime; }

	void UpdateSampleBufferSize(uint32_t bufferSize)
	{
		m_SampleBuffer.resize(bufferSize);
	}
};