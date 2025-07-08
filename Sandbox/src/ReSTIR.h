#pragma once

#include "Include.h"

#include "PointLight.h"
#include "Ray.h"
#include "Utils.h"

struct Sample
{
	// Sample path
	bool hit;
	float hitDistance;
	glm::vec3 hitPosition;
	glm::vec3 hitNormal;
	glm::vec3 hitPrevPosition;
	glm::vec3 hitPrevNormal;
	
	float lightDistance;
	glm::vec3 lightDirection;
	PointLight light;

	glm::vec3 cameraPosition;
	float BRDF;

	// ReSTIR
	float weight;
	float pdf;
	float contribution;

	Sample() = default;
	Sample(const HitInfo& hitInfo, const glm::vec3& cameraOrigin, const PointLight& pointLight, float weight, float pdf);
	Sample(const Sample& sample, float weight);

	void ReplaceLight(const PointLight& newLight);
private:
	void SetContribution()
	{
		contribution = CalcContribution((BRDF * light.emmission) / (lightDistance * lightDistance));
	}

	inline float CalcContribution(const glm::vec3& targetDistribution)
	{
		return std::max(targetDistribution.r, std::max(targetDistribution.g, targetDistribution.b));
	}
};

class Resevoir
{
public:
	float WeightSampleOut;
private:
	Sample m_Sample;
	uint32_t m_SampleCount;
	float m_WeightTotal;
public:
	Resevoir();
	Resevoir(const Sample& initialSample, float totalWeight, uint32_t totalSampleCount);

	void Update(const Sample& sample, float weight, uint32_t& seed);
	static Resevoir CombineBiased(const Resevoir& originalResevoir, const Resevoir& newResevoir, uint32_t& seed);

	Sample GetSample() const { return m_Sample; }
	Sample& GetSampleRef() { return m_Sample; }
	void SetSample(const Sample& sample) { m_Sample = sample; }

	int GetSampleCount() const { return m_SampleCount; }
	void SetSampleCount(uint32_t sampleCount) { m_SampleCount = sampleCount; }

	float GetWeightTotal() const { return m_WeightTotal; }
};