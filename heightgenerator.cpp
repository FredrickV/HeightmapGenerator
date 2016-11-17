/****************************************************************
* Name:       heightgenerator.cpp
* Purpose:    16 BPP Multi threaded height map generator
* Author:     Fredrick Vamstad
* Created:    2016 - November
* Copyright:  Public Domain
* Dependency: GLM, Simplex, C++ 11 or newer
****************************************************************/

#include "heightgenerator.h"

#define GLM_FORCE_SWIZZLE 
#pragma warning(push, 0)
#include <glm/glm.hpp>
#include "Simplex.h"
#pragma warning(pop)

#include <chrono>
#include <climits>
#include <random>

//#define USE_DEVIL_LIBRARY

#ifdef USE_DEVIL_LIBRARY
#pragma comment(lib, "DevIL.lib")
#ifdef _UNICODE
#undef _UNICODE
#include <IL/il.h>
#define _UNICODE
#else 
#include <IL/il.h>
#endif
#endif

static std::random_device EntropySrc;
static std::mt19937 RandGen(EntropySrc());

// If you already uses the image library somewhere else in the code, uncomment this line
// #define DEVIL_INIT_ELSEWHERE

typedef unsigned short GLushort;

enum DurationType {
	DurationTypeNano,
	DurationTypeMSec,
	DurationTypeSeconds,
	DurationTypeMinutes,
	DurationTypeHours
};

static inline void Wait(const int duration, const DurationType durationType = DurationTypeMSec) {
	switch (durationType) {
	case DurationTypeNano:
		std::this_thread::sleep_for(std::chrono::nanoseconds(duration)); break;
	case DurationTypeMSec:
		std::this_thread::sleep_for(std::chrono::milliseconds(duration)); break;
	case DurationTypeSeconds:
		std::this_thread::sleep_for(std::chrono::seconds(duration)); break;
	case DurationTypeMinutes:
		std::this_thread::sleep_for(std::chrono::minutes(duration)); break;
	case DurationTypeHours:
		std::this_thread::sleep_for(std::chrono::hours(duration)); break;
	}
}

unsigned int HeightGenerator::GenSeed()
{
	std::uniform_int_distribution<unsigned int> digit(0, UINT_MAX);	return digit(RandGen);
}

HeightGenerator::HeightGenerator() : m_generatedData(nullptr), m_generatedPixels(-1), m_generatedSeedUsed(0)
{
#ifdef USE_DEVIL_LIBRARY
#ifndef DEVIL_INIT_ELSEWHERE
	static bool dInit = false;
	if (!dInit) {
		ilInit();
		dInit = true;
	}
#endif 
#endif
}

HeightGenerator::~HeightGenerator()
{
	freeGeneratedData();
}

bool HeightGenerator::generate(const unsigned int seed, const int resolution,
							   const float gain, const int octaves,
							   const float scale,
							   const std::string &rawOutput,
							   const std::string &pngOutput)
{
	freeGeneratedData();

    m_generatedSeedUsed = seed;
	
	height_map_param_t hmp(resolution, gain, octaves, scale);
	m_generatedParam = hmp;

	Simplex::seed(seed);

	m_generatedPixels = hmp.resolution*hmp.resolution;

	m_generatedData = new UInt16Type[m_generatedPixels];
	if (!m_generatedData)
		return false;

	const int numCPUs = glm::clamp(std::thread::hardware_concurrency() - 1u, 1u, 64u);
	assert(numCPUs > 0); // Asserts on outdated platform/compiler

	// Medium/Large height maps, use x CPU threads
	if (numCPUs > 1 && hmp.resolution >= 1024) {
		int useCPUCount = numCPUs;
		switch (hmp.resolution) {
		case 1024:	// Don't need more than 2
			useCPUCount = 2;
			break;
		case 2048:	// Don't need more than 4
			useCPUCount = numCPUs >= 4 ? 4 : numCPUs;
			break;
		default: // Take as many as we can get
			;
		}
		std::vector<std::unique_ptr<thread_info>> threadSplits;
		int x = 0;
		int y = 0;
		const auto indexesPerCPU = m_generatedPixels / useCPUCount + 1;

		for (int i = 0; i < useCPUCount; ++i) {
			std::vector<std::pair<const int, const int>> heightMapIndexes;
			heightMapIndexes.reserve(indexesPerCPU);
			for (; x < hmp.resolution; ++x) {
				for (y = 0; y < hmp.resolution; ++y) {
					heightMapIndexes.push_back(std::pair<const int, const int>(x, y));
					if (static_cast<const int>(heightMapIndexes.size()) >= indexesPerCPU)
						break;
				}
				if (static_cast<const int>(heightMapIndexes.size()) >= indexesPerCPU)
					break;
			}
			threadSplits.push_back(std::unique_ptr<thread_info>(new thread_info(this, m_generatedData, hmp, std::move(heightMapIndexes))));
		}
		while (true) {
			size_t completed = 0;
			for (const auto & i : threadSplits) {
				if (i->threadState == ThreadStateProcessingDone) {
					if (i->thread && i->thread->joinable()) {
						i->thread->join();;
					}
					i->threadState = ThreadStateDone;
				}
				else if (i->threadState == ThreadStateDone)
					++completed;
			}
			if (completed == threadSplits.size()) {
				break;
			}
			Wait(10);
		}
	}
	else {
		// Small height maps, use only 1 CPU thread
		std::vector<std::pair<const int, const int>> heightMapIndexes;
		heightMapIndexes.reserve(m_generatedPixels);

		for (int x = 0; x < hmp.resolution; ++x) {
			for (int y = 0; y < hmp.resolution; ++y) {
				heightMapIndexes.push_back(std::pair<const int, const int>(x, y));
			}
		}
		generationHeight(hmp, m_generatedData, heightMapIndexes);
	}
	int errors = 0;

	if (rawOutput.length()) {
		FILE *fp = fopen(rawOutput.c_str(), "wb");
		if (fp) {
			unsigned char version = 1;
			unsigned short res = (unsigned short)hmp.resolution;

			fwrite(&version, sizeof(version), 1, fp);
			fwrite(&res, sizeof(res), 1, fp);
			errors += fwrite(m_generatedData, m_generatedPixels * sizeof(UInt16Type), 1, fp) != 1;
			fclose(fp);
		}
	}

#ifdef USE_DEVIL_LIBRARY
	if (pngOutput.length()) {
		ILuint imageID = ilGenImage();
		ilBindImage(imageID);

		ilTexImage(
			hmp.resolution,
			hmp.resolution,
			0,
			1,
			IL_LUMINANCE,
			IL_UNSIGNED_SHORT,
			m_generatedData
		);
		ilEnable(IL_FILE_OVERWRITE);
		ilSave(IL_PNG, pngOutput.c_str());

        ilDeleteImage(imageID);
	}
#endif
	return !errors;
}

void HeightGenerator::freeGeneratedData()
{
	if (m_generatedData) {
		delete[] m_generatedData;
		m_generatedData = nullptr;
	}
	m_generatedPixels = 0;
	m_generatedParam = height_map_param_t();
    m_generatedSeedUsed = 0;
}

void HeightGenerator::generationHeight(const HeightGenerator::height_map_param_t &hmp, UInt16Type *data, const std::vector<std::pair<const int, const int>> &workSet)
{
	for (const auto & i : workSet) {
		glm::vec2 position = (glm::vec2(i.first, i.second)) * hmp.scale;
        float n = Simplex::iqMatfBmEx(position, (uint8_t)hmp.octaves, glm::mat2(2.3f, -1.5f, 1.5f, 2.3f), hmp.gain);
        n *= Simplex::ridgedMF(position * glm::vec2(0.75f), 0.95f, hmp.octaves, 1.5f, 0.33f);
        n -= Simplex::worleyfBm(position, 12, 2.0f, hmp.gain);

        // World move "down" 0.30
		data[i.first + i.second*hmp.resolution] = (UInt16Type)(glm::clamp(double(n* 0.5f + 0.20), 0.0, 1.0) * 65535.0);
	}
}

void HeightGenerator::generationHeightMapMultiThread(HeightGenerator::thread_info *threadInfo, UInt16Type *data, const std::vector<std::pair<const int, const int>> &workSet)
{
    generationHeight(threadInfo->params, data, workSet);
	threadInfo->threadState = ThreadStateProcessingDone;
}