// implementation is from libavcodec
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <format>
#include <vector>
#include <unordered_map>
#include <fstream>

constexpr int8_t ff_adpcm_index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

constexpr int16_t ff_adpcm_step_table[89] = {
        7,     8,     9,    10,    11,    12,    13,    14,    16,    17,
       19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
       50,    55,    60,    66,    73,    80,    88,    97,   107,   118,
      130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
      337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
      876,   963,  1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
     2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
     5894,  6484,  7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

class AdpcmEncoder {
public:
    int16_t prev_sample = 0;
    int step_index = 0;

    uint8_t EncodeSample(int16_t sample);
};

inline uint8_t AdpcmEncoder::EncodeSample(int16_t sample) {
    int best_code = 0;
    int best_error = INT_MAX;
    int best_pred = 0;
    int step = ff_adpcm_step_table[step_index];
    
    for (int code = 0; code < 16; ++code) {
        int test_diff = step >> 3;
        if (code & 4) test_diff += step;
        if (code & 2) test_diff += step >> 1;
        if (code & 1) test_diff += step >> 2;
        if (code & 8) test_diff = -test_diff;

        int predicted = prev_sample + test_diff;
        predicted = std::clamp(predicted, -32768, 32767);

        int error = std::abs(sample - predicted);
        if (error < best_error) {
            best_error = error;
            best_code = code;
            best_pred = predicted;
        }
    }

    prev_sample = best_pred;
    step_index += ff_adpcm_index_table[best_code];
    step_index = std::clamp(step_index, 0, 88);

    return static_cast<uint8_t>(best_code & 0xF);
}

class AdpcmDecoder {
public:
    int DecodeSamples(const char* inputPtr, char* outPtr, int numSamples, uint16_t initPredict, uint8_t initStep) { // this is yoinked from flipnote adpcm itself
        int iVar1;
        short sVar2;
        int step;
        int pred;
        int diff;
        int stepIndex;
        uint8_t currByte;
        uint8_t sample;
        bool loNibble = false;
        const int16_t* stepTablePtr = ff_adpcm_step_table;

        pred = initPredict;
        stepIndex = initStep;
        iVar1 = numSamples << 1;
        sVar2 = stepTablePtr[stepIndex];
        if (numSamples != 0) {
            do {
                step = sVar2;
                if (loNibble) {
                    sample = currByte >> 4;
                }
                else {
                    sample = static_cast<uint8_t>(*inputPtr);
                    currByte = sample;
                    inputPtr++;
                }
                loNibble = !loNibble;
                stepIndex += ff_adpcm_index_table[sample & 0xf];
                if (stepIndex < 0) {
                    stepIndex = 0;
                }
                if (stepIndex > 88) {
                    stepIndex = 88;
                }

                diff = step >> 3;
                if (sample & 4) {
                    diff += step;
                }
                if (sample & 2) {
                    diff += (step >> 1);
                }
                if (sample & 1) {
                    diff += (step >> 2);
                }
                if (sample & 8) {
                    diff = -diff;
                }

                diff = pred + diff;
                pred = std::clamp(diff, -32768, 32767);

                *reinterpret_cast<short*>(outPtr) = static_cast<short>(pred);
                sVar2 = stepTablePtr[stepIndex];
                numSamples--;
                outPtr += 2;
            } while (numSamples != 0);
        }

        return iVar1;
    }
};

#pragma pack(push, 1)
struct WAVHeader {
    char riff[4] = { 'R', 'I', 'F', 'F' };
    uint32_t overall_size;
    char wave[4] = { 'W', 'A', 'V', 'E' };
    char fmt[4] = { 'f', 'm', 't', ' ' };
    uint32_t fmt_chunk_size = 16;
    uint16_t audio_format = 1;  // PCM format
    uint16_t num_channels = 1;  // Mono
    uint32_t sample_rate = 8192;
    uint32_t byte_rate;
    uint16_t block_align;  // NumChannels * BitsPerSample / 8
    uint16_t bits_per_sample = 16;  // 16-bit samples
    char data[4] = { 'd', 'a', 't', 'a' };
    uint32_t data_size;
};
#pragma pack(pop)

inline void writeWavHeader(std::ofstream& outFile, uint32_t dataSize) {
    WAVHeader header;
    header.byte_rate = header.sample_rate * header.num_channels * (header.bits_per_sample / 8);
    header.block_align = header.num_channels * (header.bits_per_sample / 8);
    header.data_size = dataSize;
    header.overall_size = 36 + dataSize;
    outFile.write(reinterpret_cast<char*>(&header), sizeof(header));
}

struct AdpcmState {
    int16_t pred;
    uint8_t stepIndex;
};

// Encodes a single PCM sample and finds the best initial step index based on closest reconstruction.
static AdpcmState EncodeInitialState(int16_t sample) {
    AdpcmState bestState = { 0, 0 };
    int minError = std::numeric_limits<int>::max();

    //std::cout << "Original Sample: " << sample << std::endl;

    for (uint8_t stepIndex = 0; stepIndex < 89; ++stepIndex) {
        AdpcmEncoder encoder;
        encoder.prev_sample = 0;
        encoder.step_index = stepIndex;

        // Encode the sample twice (Flipnote behavior)
        char encodedByte = encoder.EncodeSample(sample) | (encoder.EncodeSample(sample) << 4);

        AdpcmDecoder decoder;
        char decodedBytes[4] = {};
        decoder.DecodeSamples(&encodedByte, decodedBytes, 2, 0, stepIndex);
        int16_t decodedSample = static_cast<uint8_t>(decodedBytes[0]) |
            (static_cast<uint8_t>(decodedBytes[1]) << 8);

        //std::cout << "StepIndex: " << static_cast<int>(stepIndex) << " | Decoded Sample: " << decodedSample << std::endl;

        int error = std::abs(decodedSample - sample);

        // Track best match
        if (error < minError) {
            minError = error;
            bestState.pred = 0;
            bestState.stepIndex = stepIndex;
        }
    }

    return bestState;
}