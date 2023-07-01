#pragma once
#ifndef ALMIGHTY_HASH_H
#define ALMIGHTY_HASH_H

#include <cstdint>
#include <vector>

namespace almighty_hash {

    // Constants
    extern const __uint128_t P;
    extern const __uint128_t Q;
    extern const __uint128_t R;
    extern const __uint128_t A;
    extern const __uint128_t B;
    extern const __uint128_t C;
    extern const __uint128_t D;
    extern const __uint128_t invP_PQ;
    extern const __uint128_t invQ_PQ;
    extern const __uint128_t invP_PR;
    extern const __uint128_t invR_PR;

    // Function to compute the Chinese Remainder Theorem (CRT) element
    __uint128_t crt_element(__uint128_t x, __uint128_t y, __uint128_t mod1, __uint128_t mod2,
                            __uint128_t r1, __uint128_t r2, __uint128_t inv_mod1, __uint128_t inv_mod2);

    // The main hash function
    void newdisco_64(const void* key, int len, uint32_t seed, void* out);

} // namespace almighty_hash

#endif // ALMIGHTY_HASH_H

