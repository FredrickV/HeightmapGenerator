/****************************************************************
* Name:       heightgenerator.h
* Purpose:    16 BPP Multi threaded height map generator
* Author:     Fredrick Vamstad
* Created:    2016 - November
* Copyright:  Public Domain
* Dependency: GLM, Simplex, C++ 11 or newer
****************************************************************/


#ifndef HEIGHT_GENERATOR_H
#define HEIGHT_GENERATOR_H

#include <thread>
#include <atomic>
#include <vector>
#include <string>

class HeightGenerator
{
private:
	typedef unsigned short UInt16Type;
	enum ThreadState : short {
		ThreadStateIdle = 0,
		ThreadStateProcessing,
		ThreadStateProcessingDone,
		ThreadStateDone
	};

public:
	typedef struct height_map_param_t {
		height_map_param_t(const int resolutionInp,
			const float gainInp,
			const int octaveInput,
			const float scaleInput) : resolution(resolutionInp),
			gain(gainInp), octaves(octaveInput), scale(scaleInput) {}
		height_map_param_t() : resolution(0), gain(0.0f), octaves(0), scale(0.0f) {}
		int resolution;
		float gain;
		int octaves;
		float scale;
	}height_map_param_t;


	static unsigned int GenSeed();

	HeightGenerator();
	~HeightGenerator();

	// Notice: Height maps with a resolution above 8192 x 8192 requires a 64 bit build
	// Example input: GenSeed(), ~1-4 k res, ~0.3 - 0.35 gain, ~20 Octaves, ~ 0.001 scale
	bool generate(const unsigned int seed, const int resolution,
				  const float gain, const int octaves,
				  const float scale, 
				  // Disable : Leave blank
				  const std::string &rawOutput = "",
				  const std::string &pngOutput = "");

	inline const UInt16Type * generatedData() {
		return m_generatedData;
	}
	inline int generatedPixels() {
		return m_generatedPixels;
	}

	inline const height_map_param_t &generatedParam() {
		return m_generatedParam;
	}

    inline unsigned int generatedSeed() {
        return m_generatedSeedUsed;
    }

    bool saveGeneratedData(const std::string &savePath);
    bool loadGeneratedData(const std::string &loadPath);

	void freeGeneratedData();

private:
	class thread_info {
	public:
		thread_info(HeightGenerator *self,
			UInt16Type *data,
			const height_map_param_t paramsInp,
            const uint32_t seedInp,
			const std::vector<std::pair<const int, const int>> &workSet) :
			params(paramsInp), seed(seedInp), threadState(ThreadStateProcessing),
			thread(new std::thread(&HeightGenerator::generationHeightMapMultiThread, self, this, data, workSet)) { }
		~thread_info() {
			if (thread) {
				delete thread;
			}
		}
		const height_map_param_t params;
        const uint32_t seed;
		std::atomic<ThreadState> threadState;

		std::thread *thread;
	};

	void generationHeight(const uint32_t seed, const height_map_param_t &hmp, UInt16Type *data, const std::vector<std::pair<const int, const int>> &workSet);
	void generationHeightMapMultiThread(thread_info *threadInfo, UInt16Type *data, const std::vector<std::pair<const int, const int>> &workSet);

	UInt16Type *m_generatedData;
	int m_generatedPixels;
	height_map_param_t m_generatedParam;
    unsigned int m_generatedSeedUsed;
};

#endif