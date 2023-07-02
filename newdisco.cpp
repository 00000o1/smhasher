// very slow and fails sparse
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <omp.h> // For OpenMP

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
    // Majestic numbers 🎩
    const __uint128_t prime = 13166748625691186689ULL;
    const __uint128_t generator = 13033477949918730441ULL;

    // Get and align the input data to 8-byte boundaries
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key);
    alignas(uint64_t) uint8_t aligned_data[len];
    memcpy(aligned_data, data, len);
    const uint64_t* data64 = reinterpret_cast<const uint64_t*>(aligned_data);

    __uint128_t state = seed + 1; // Adding 1 to avoid multiplication by 0

    // Loop through the input data blocks
    for (int i = 0; i < len / 8; i++) {
        uint64_t k = data64[i];

        // Absorbing the message block by subtraction (mod prime)
        if (state > k) {
            state -= k;
        } else {
            state = (state + prime) - k;
        }

        // Transforming the state with the generator modulo prime
        state *= generator;
        state %= prime;
    }

    // Let's take care of the remaining bytes 🍼
    int offset = (len / 8) * 8;
    uint64_t remainder = 0;
    for (int i = 0; i < len - offset; i++) {
        remainder = (remainder << 8) | aligned_data[offset + i];
    }

    // Apply the same transformation for the remaining bytes
    if (state > remainder) {
        state -= remainder;
    } else {
        state = (state + prime) - remainder;
    }
    state *= generator;
    state %= prime;

    // Convert our 128-bit state into 64-bit
    uint64_t hash = static_cast<uint64_t>(state ^ (state >> 64));

    // Copy the magic result into the output buffer
    memcpy(out, &hash, sizeof(hash));
}

