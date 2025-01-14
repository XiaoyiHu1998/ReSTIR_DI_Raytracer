#pragma once

#include "Include.h"
#include "Utils.h"

namespace ReSTIR
{
	//using T = glm::vec3;

	template <class T>
	class Resevoir
	{
	private:
		T m_Sample;
		float m_WeightTotal;

		int m_SampleCount;
	public:
		Resevoir(T initialSample, float weight):
			m_Sample{ initialSample }, m_SampleCount{ 1 }, m_WeightTotal { weight }
		{}

		T GetSample() { return m_Sample; }

		bool Update(const T& sample, float weight, uint32_t& seed)
		{
			m_SampleCount++;
			m_WeightTotal += weight;

			if (Utils::RandomFloat(seed) < weight / m_WeightTotal)
			{
				m_Sample = sample;
				return true;
			}

			return false;
		}
	};


	Resevoir<glm::vec3> ResevoirSampling(const glm::vec3& sample, int candidateCount, uint32_t& seed)
	{
		float weight = 1.0f / candidateCount;
		Resevoir<glm::vec3> resevoir(glm::vec3(0), weight);

		for (int i = 0; i < candidateCount; i++)
		{
			resevoir.Update(sample, weight, seed);
		}


		return resevoir;
	}

}