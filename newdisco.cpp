#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>

constexpr uint64_t PRIME_MULTIPLIER = 0x880355f21e6d1965ULL;

inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key);
    uint64_t hash = seed + PRIME_MULTIPLIER + len;

    // Allocate memory for alignment
    alignas(uint64_t) uint8_t aligned_data[len];
    memcpy(aligned_data, data, len);

    // Process 64-bit blocks
    int num_blocks = len / 8;
    for (int i = 0; i < num_blocks; i++) {
        uint64_t block;
        memcpy(&block, aligned_data + i * 8, 8);
        
        hash ^= block + len + i;
        hash = rotl64(hash, 13);
        hash = hash * PRIME_MULTIPLIER + 0x9E3779B97F4A7C15ULL;
    }

    // Process leftover bytes
    int offset = num_blocks * 8;
    for (int i = 0; i < len - offset; i++) {
        hash ^= aligned_data[offset + i] + len + i;
        hash = rotl64(hash, 5);
        hash = hash * PRIME_MULTIPLIER + 0xBF58476D1CE4E5B9ULL;
    }

    // Finalization
    hash ^= len;
    hash ^= (hash >> 33);
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= (hash >> 33);
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= (hash >> 33);

    memcpy(out, &hash, sizeof(hash)); // copy the result into the output buffer
}

