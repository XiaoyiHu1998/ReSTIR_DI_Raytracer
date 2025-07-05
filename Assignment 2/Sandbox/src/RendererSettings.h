#pragma once

#include "Include.h"
#include "thread"

struct RendererSettings
{
	enum class RenderMode
	{
		Normals = 0,
		TraversalSteps = 1,
		DI = 2,
		ReSTIR = 3
	};

	RenderMode Mode = RenderMode::ReSTIR;

	int ThreadCount = std::thread::hardware_concurrency();

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
	int TemporalSampleCountRatio = 15;
	float TemporalMaxDistance = 0.1f;
	float TemporalMaxDistanceDepthScaling = 0.18f;
	float TemporalMinNormalSimilarity = 0.93f;

	// Spatial Reuse
	bool EnableSpatialReuse = true;
	int SpatialReuseNeighbours = 3;
	int SpatialPixelRadius = 10;
	float SpatialMaxDistance = 0.06f;
	float SpatialMaxDistanceDepthScaling = 0.005f;
	float SpatialMinNormalSimilarity = 0.90f;

	bool operator==(const RendererSettings& otherSettings)
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

	bool operator!=(const RendererSettings& otherSettings)
	{
		return !operator==(otherSettings);
	}
};