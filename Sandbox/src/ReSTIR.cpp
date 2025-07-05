#include "ReSTIR.h"


Sample::Sample(const HitInfo& hitInfo, const glm::vec3& cameraOrigin, const PointLight& pointLight, float weight, float pdf)
{
	hit = hitInfo.hit;
	hitDistance = hitInfo.distance;
	hitPosition = hitInfo.location;
	hitNormal = hitInfo.normal;
	cameraPosition = cameraOrigin;
	light = pointLight;
	this->weight = weight;
	this->pdf = pdf;

	lightDirection = light.position - hitPosition;
	lightDistance = glm::length(lightDirection);
	lightDirection = glm::normalize(lightDirection);
	BRDF = glm::dot(hitNormal, lightDirection);

	SetContribution();
}

Sample::Sample(const Sample& sample, float weight)
{
	hit = sample.hit;
	hitDistance = sample.hitDistance;
	hitPosition = sample.hitPosition;
	hitNormal = sample.hitNormal;
	cameraPosition = sample.cameraPosition;
	light = sample.light;
	this->weight = weight;
	pdf = sample.pdf;

	lightDirection = sample.lightDirection;
	lightDistance = sample.lightDistance;
	BRDF = sample.BRDF;

	SetContribution();
}

void Sample::ReplaceLight(const PointLight& newLight)
{
	light = newLight;

	lightDirection = light.position - hitPosition;
	lightDistance = glm::length(lightDirection);
	lightDirection = glm::normalize(lightDirection);
	BRDF = glm::dot(hitNormal, lightDirection);

	SetContribution();
}

Resevoir::Resevoir() :
	m_Sample{ Sample() }, m_SampleCount{ 0 }, m_WeightTotal{ 0.0f }, WeightSampleOut{ 0.0f }
{}

Resevoir::Resevoir(const Sample& initialSample, float totalWeight, uint32_t totalSampleCount) :
	m_Sample{ initialSample }, m_WeightTotal{ totalWeight }, m_SampleCount{ totalSampleCount }
{}

void Resevoir::Update(const Sample& sample, float weight, uint32_t& seed)
{
	m_SampleCount++;
	m_WeightTotal += weight;

	if (Utils::RandomFloat(seed) <= weight / m_WeightTotal)
		m_Sample = sample;
}

Resevoir Resevoir::CombineBiased(const Resevoir& originalResevoir, const Resevoir& newResevoir, uint32_t& seed)
{
	Resevoir combinedResevoir;

	const Sample& originalSample = originalResevoir.GetSample();
	const Sample& newSample = newResevoir.GetSample();

	float originalWeight = originalSample.contribution * originalResevoir.WeightSampleOut * originalResevoir.GetSampleCount();
	float newWeight = newSample.contribution * newResevoir.WeightSampleOut * newResevoir.GetSampleCount();

	combinedResevoir.Update(originalSample, originalWeight, seed);
	combinedResevoir.Update(newSample, newWeight, seed);

	combinedResevoir.SetSampleCount(originalResevoir.GetSampleCount() + newResevoir.GetSampleCount());
	combinedResevoir.WeightSampleOut = (1.0f / combinedResevoir.GetSampleRef().contribution) * (combinedResevoir.GetWeightTotal() / static_cast<float>(combinedResevoir.GetSampleCount()));

	return combinedResevoir;
}