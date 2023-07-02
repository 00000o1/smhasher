#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

const uint64_t PRIME_P = 410105453987440927;
const uint64_t GENERATOR_P = 122507474825287071;
const uint64_t PRIME_Q = 577898295939987293;
const uint64_t GENERATOR_Q = 534712801256041965;

inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
    // Get and align the input data to 8-byte boundaries
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key);
    alignas(uint64_t) uint8_t aligned_data[len];
    memcpy(aligned_data, data, len);
    const uint64_t* data64 = reinterpret_cast<const uint64_t*>(aligned_data);

    uint64_t state_p = 2 + (len + 1) * seed;
    uint64_t state_q = 1 + (len - 1) ^ seed;
    uint64_t temp = 0;

    // Processing 128-bit (two 64-bit blocks) per turn
    for (int i = 0; i < len / 8; i+=2) {
        temp = data64[i] % PRIME_P;
        temp *= GENERATOR_P;
        temp %= PRIME_P;
        state_p += temp;
        temp = data64[i+1] % PRIME_Q;
        temp *= GENERATOR_Q;
        temp %= PRIME_Q;
        state_q += temp;
    }

    // Processing remaining bytes
    uint64_t remainder_p = 0;
    uint64_t remainder_q = 0;
    int offset = (len / 16) * 16;
    for (int i = 0; i < len - offset; i += 2) {
        remainder_p |= static_cast<uint64_t>(aligned_data[offset + i]) << (8 * i);
        remainder_q |= static_cast<uint64_t>(aligned_data[offset + i + 1]) << (8 * i);
    }
    if (len - offset > 0) {
        temp = remainder_p % PRIME_P;
        temp *= GENERATOR_P;
        temp %= PRIME_P;
        state_p += temp;
        temp = remainder_q % PRIME_Q;
        temp *= GENERATOR_Q;
        temp %= PRIME_Q;
        state_q += temp;
    }
    state_p = ((state_p + len) * GENERATOR_P) % PRIME_P;
    state_q = ((state_q + len) * GENERATOR_Q) % PRIME_Q;
    state_p = ((state_p - len) * GENERATOR_P) % PRIME_P;
    state_q = ((state_q - len) * GENERATOR_Q) % PRIME_Q;

    uint64_t result = 0;
    result -= state_p;
    result -= state_q;

    memcpy(out, &result, sizeof(result));
}

