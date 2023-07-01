#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <execution>
#include <future>
#include <vector>
#include <omp.h> // For OpenMP

constexpr uint64_t PRIME_MULTIPLIER = 0x880355f21e6d1965ULL;

inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key);

    uint64_t hash = seed + PRIME_MULTIPLIER;

    // When the input has enough strength to conjure the spirits of OpenMP!
    if (len >= 256) {
        #pragma omp parallel for reduction(^: hash)
        for (int i = 0; i < len / 8; i++) {
            uint64_t block;
            memcpy(&block, data + i * 8, 8);

            hash ^= block;
            hash = rotl64(hash, 13);
            hash = hash * PRIME_MULTIPLIER + 0x9E3779B97F4A7C15ULL;
        }
    } else {
        // For the whispers of small input, a lone warrior is dispatched!
        for (int i = 0; i < len / 8; i++) {
            uint64_t block;
            memcpy(&block, data + i * 8, 8);

            hash ^= block;
            hash = rotl64(hash, 13);
            hash = hash * PRIME_MULTIPLIER + 0x9E3779B97F4A7C15ULL;
        }
    }

    // Brave remainders, your valor shall not go unnoticed!
    uint64_t remainder_hash = 0;
    int offset = (len / 8) * 8;
    for (int i = 0; i < len - offset; i++) {
        remainder_hash ^= data[offset + i];
        remainder_hash = rotl64(remainder_hash, 5);
        remainder_hash = remainder_hash * PRIME_MULTIPLIER + 0xBF58476D1CE4E5B9ULL;
    }
    hash ^= remainder_hash;

    // And thus the magic is sealed!
    hash ^= len;
    hash ^= (hash >> 33);
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= (hash >> 33);
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= (hash >> 33);

    memcpy(out, &hash, sizeof(hash));
}


