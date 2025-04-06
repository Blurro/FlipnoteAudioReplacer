// implementation is from libavcodec
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>

static void ApplyFadeIn(int16_t* sample_data, uint64_t pcm_frame_count, float fade_duration_sec, int sample_rate) {
    int fade_samples = fade_duration_sec * sample_rate;

    for (int i = 0; i < pcm_frame_count; ++i) {
        if (i < fade_samples) {
            float fade_factor = float(i) / fade_samples;
            sample_data[i] = int16_t(sample_data[i] * fade_factor);
        }
        else {
            break;
        }
    }
}

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

    int step = ff_adpcm_step_table[step_index];
    int diff = sample - prev_sample;

    int code = 0;
    if (diff < 0) {
        code = 8;
        diff = -diff;
    }

    if (diff >= step) { code |= 4; diff -= step; }
    if (diff >= step / 2) { code |= 2; diff -= step / 2; }
    if (diff >= step / 4) { code |= 1; }

    int delta = step >> 3;
    if (code & 1) delta += step >> 2;
    if (code & 2) delta += step >> 1;
    if (code & 4) delta += step;
    if (code & 8) delta = -delta;

    prev_sample += delta;
    prev_sample = std::clamp(prev_sample, static_cast<int16_t>(-32768), static_cast<int16_t>(32767));

    step_index += ff_adpcm_index_table[code];
    step_index = std::clamp(step_index, 0, 88);

    return static_cast<uint8_t>(code & 0x0F);
}