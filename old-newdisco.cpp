#include <cstdint>
#include <vector>
#include <iostream>
#include "math_utils.h"


#include <cstdint>
#include <cstring>
#include <vector>

namespace almighty_hash {

    const uint64_t R = 15334707568420170289ULL; // prime
    const uint64_t S = 4667929651900131889ULL;  // prime

    const uint64_t C = 7298659488951121034ULL;  // generator modulo R
    const uint64_t D = 986140072491549284ULL;   // generator modulo S

    void fast_hash_function(const void* key, int len, uint32_t seed, void* out) {
        alignas(uint64_t) const uint8_t *data = (const uint8_t *)key;
        alignas(uint64_t) uint64_t *hash_value = (uint64_t *)out;

        uint64_t stateR = seed;
        uint64_t stateS = seed;

        for (int i = 0; i + 8 <= len; i += 8) {
            alignas(uint64_t) char buffer[8];
            std::memcpy(buffer, data + i, 8);
            uint64_t message_block = *reinterpret_cast<const uint64_t*>(buffer);

            // Move message block into the state by XOR
            stateR ^= message_block;
            stateS ^= message_block;

            // Multiply by the generator modulo the prime
            stateR = (stateR * C) % R;
            stateS = (stateS * D) % S;
        }

        // Combine in the finalization
        *hash_value = stateR + stateS;
    }
}

