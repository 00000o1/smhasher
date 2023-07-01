#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <omp.h> // For OpenMP

constexpr uint64_t PRIME_MULTIPLIER = 0x880355f21e6d1965ULL;

inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

#include <omp.h> // For OpenMP

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key);

    uint64_t hash = seed + PRIME_MULTIPLIER;

    // Align the input data to 8-byte boundaries
    alignas(8) uint8_t aligned_data[len];
    memcpy(aligned_data, data, len);
    const uint64_t* data64 = reinterpret_cast<const uint64_t*>(aligned_data);

    // Invoke the mighty OpenMP when the input length is worthy of its prowess!
    if (len >= 256) {
        #pragma omp parallel for reduction(^: hash)
        for (int i = 0; i < len / 8; i++) {
            uint64_t block = data64[i];

            hash ^= block;
            hash = rotl64(hash, 13);
            hash = hash * PRIME_MULTIPLIER + 0x9E3779B97F4A7C15ULL;
        }
    } else {
        // Tread the path of a single warrior when the input is but a mere whisper in the wind
        for (int i = 0; i < len / 8; i++) {
            uint64_t block = data64[i];

            hash ^= block;
            hash = rotl64(hash, 13);
            hash = hash * PRIME_MULTIPLIER + 0x9E3779B97F4A7C15ULL;
        }
    }

    // Unwavering focus, as we deal with the remainders!
    uint64_t remainder_hash = 0;
    int offset = (len / 8) * 8;
    for (int i = 0; i < len - offset; i++) {
        remainder_hash ^= aligned_data[offset + i];
        remainder_hash = rotl64(remainder_hash, 5);
        remainder_hash = remainder_hash * PRIME_MULTIPLIER + 0xBF58476D1CE4E5B9ULL;
    }
    hash ^= remainder_hash;

    // Grand finale!
    hash ^= len;
    hash ^= (hash >> 33);
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= (hash >> 33);
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= (hash >> 33);

    memcpy(out, &hash, sizeof(hash));
}

