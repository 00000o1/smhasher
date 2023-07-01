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

void process_chunk(const uint8_t* data, int len, unsigned seed, uint64_t& intermediate_hash) {
    uint64_t hash = seed + PRIME_MULTIPLIER;

    int num_blocks = len / 8;
    for (int i = 0; i < num_blocks; i++) {
        uint64_t block;
        memcpy(&block, data + i * 8, 8);

        hash ^= block;
        hash = rotl64(hash, 13);
        hash = hash * PRIME_MULTIPLIER + 0x9E3779B97F4A7C15ULL;
    }

    int offset = num_blocks * 8;
    for (int i = 0; i < len - offset; i++) {
        hash ^= data[offset + i];
        hash = rotl64(hash, 5);
        hash = hash * PRIME_MULTIPLIER + 0xBF58476D1CE4E5B9ULL;
    }

    intermediate_hash ^= hash;
}


void newdisco_64(const void* key, int len, unsigned seed, void* out) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key);

    uint64_t hash = seed + PRIME_MULTIPLIER;

    #pragma omp parallel reduction(^: hash)
    {
        uint64_t thread_hash = 0;
        #pragma omp for
        for (int i = 0; i < len; i += 8) {
            uint64_t block;
            memcpy(&block, data + i, 8);

            thread_hash ^= block;
            thread_hash = rotl64(thread_hash, 13);
            thread_hash = thread_hash * PRIME_MULTIPLIER + 0x9E3779B97F4A7C15ULL;
        }

        hash ^= thread_hash;
    }

    // Finalization
    hash ^= len;
    hash ^= (hash >> 33);
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= (hash >> 33);
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= (hash >> 33);

    memcpy(out, &hash, sizeof(hash));
}

