#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <omp.h>

constexpr uint64_t PRIME_MULTIPLIER1 = 0xC6A4A7935BD1E995;
constexpr uint64_t PRIME_MULTIPLIER2 = 0x369DEA0F31A53F85;
constexpr uint64_t ROTATE_BITS = 27;

inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key);

    uint64_t hash = seed ^ (len * PRIME_MULTIPLIER1);

    const uint64_t* chunks = reinterpret_cast<const uint64_t*>(data);
    int num_chunks = len / 8;

    // parallelize for inputs above a certain size
    if (len >= 256) {
        #pragma omp parallel for reduction(^: hash)
        for (int i = 0; i < num_chunks; i++) {
            uint64_t k = chunks[i];
            k *= PRIME_MULTIPLIER1;
            k ^= k >> 47;
            k *= PRIME_MULTIPLIER2;
            hash ^= k;
            hash = rotl64(hash, ROTATE_BITS);
            hash = hash * PRIME_MULTIPLIER1 + PRIME_MULTIPLIER2;
        }
    } else {
        for (int i = 0; i < num_chunks; i++) {
            uint64_t k = chunks[i];
            k *= PRIME_MULTIPLIER1;
            k ^= k >> 47;
            k *= PRIME_MULTIPLIER2;
            hash ^= k;
            hash = rotl64(hash, ROTATE_BITS);
            hash = hash * PRIME_MULTIPLIER1 + PRIME_MULTIPLIER2;
        }
    }

    // deal with non-block sized input left over
    const uint8_t* tail = data + (num_chunks * 8);
    uint64_t remainder = 0;
    for (int i = 0; i < len % 8; i++) {
        remainder |= uint64_t(tail[i]) << (i * 8);
    }
    remainder *= PRIME_MULTIPLIER1;
    remainder ^= remainder >> 47;
    remainder *= PRIME_MULTIPLIER2;
    hash ^= remainder;
    hash = rotl64(hash, ROTATE_BITS);
    hash = hash * PRIME_MULTIPLIER1 + PRIME_MULTIPLIER2;

    // finalize
    hash ^= hash >> 33;
    hash *= PRIME_MULTIPLIER2;
    hash ^= hash >> 29;
    hash *= PRIME_MULTIPLIER1;
    hash ^= hash >> 18;

    memcpy(out, &hash, sizeof(hash));
}

