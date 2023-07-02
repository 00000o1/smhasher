#include <cstdint>
#include <cstdlib>
#include <cstring>

const uint64_t PRIME_P = 15334707568420170289ULL;
const uint64_t GENERATOR_P = 1888816376ULL;

const uint64_t PRIME_Q = 15334707568420170289ULL;
const uint64_t GENERATOR_Q = 3034954066ULL;

// Rotate left function
inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
    // Get and align the input data to 8-byte boundaries
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key);
    alignas(uint64_t) uint8_t aligned_data[len];
    memcpy(aligned_data, data, len);
    const uint64_t* data64 = reinterpret_cast<const uint64_t*>(aligned_data);

    uint64_t stateP = len;
    uint64_t stateQ = len;
    stateP = (stateP << 32 | (seed + len)) - seed;
    stateQ = (stateQ << 32 | (seed + len)) - seed;

    // Processing 128-bit (two 64-bit blocks) per iteration
    for (int i = 0; i < len / 16; i += 2) {
        stateP = ((stateP + data64[i]) * GENERATOR_P) % PRIME_P;
        stateQ = ((stateQ + data64[i + 1]) * GENERATOR_Q) % PRIME_Q;
    }

    // Processing remaining bytes
    uint64_t remainderP = 0;
    uint64_t remainderQ = 0;
    int offset = (len / 16) * 16;
    for (int i = 0; i < len - offset; i++) {
        if (i < 8) {
            remainderP |= static_cast<uint64_t>(aligned_data[offset + i]) << (8 * i);
        } else {
            remainderQ |= static_cast<uint64_t>(aligned_data[offset + i]) << (8 * (i - 8));
        }
    }

    if (len - offset > 0) {
        if (len - offset <= 8) {
            stateP = ((stateP + remainderP) * GENERATOR_P) % PRIME_P;
        } else {
            stateP = ((stateP + remainderP) * GENERATOR_P) % PRIME_P;
            stateQ = ((stateQ + remainderQ) * GENERATOR_Q) % PRIME_Q;
        }
    }

    // Combine the low 32 bits of stateP and stateQ to make 64-bit output hash
    uint64_t hash = (stateP & 0xFFFFFFFF) | ((stateQ & 0xFFFFFFFF) << 32);
    memcpy(out, &hash, sizeof(hash));
}

