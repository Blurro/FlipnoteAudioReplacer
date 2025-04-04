// implementation is from libavcodec
#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>

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
    int32_t delta = sample - prev_sample;
    int32_t enc_sample = 0;

    if (delta < 0) {
        enc_sample = 8;
        delta = -delta;
    }
    enc_sample += std::min(7, delta * 4 / ff_adpcm_step_table[step_index]);
    prev_sample = sample;
    step_index = std::clamp(step_index + ff_adpcm_index_table[enc_sample], 0, 79);
    return enc_sample;
}