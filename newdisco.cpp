#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>

namespace almighty_hash {
  const uint64_t R = 15334707568420170289ULL; // prime
  const uint64_t S = 4667929651900131889ULL;  // prime

  const uint64_t C = 7298659488951121034ULL;  // generator modulo R
  const uint64_t D = 986140072491549284ULL;   // generator modulo S

  void newdisco_64(const void* key, int len, uint32_t seed, void* out) {
    const uint8_t *input_data = (const uint8_t *)key;
    uint64_t *hash_value = (uint64_t *)out;

    // Copy input data to an aligned buffer for efficient memory access
    alignas(uint64_t) std::vector<uint8_t> aligned_buffer(input_data, input_data + len);
    const uint8_t *aligned_data = aligned_buffer.data();

    uint64_t stateR = 0xf00baa55 - seed;
    uint64_t stateS = ~seed;

    std::cout << "len " << len << std::endl;
    // Process each 64-bit block of the input data
    int i;
    for (i = 0; i + 8 <= len; i += 8) {
      // Load the current message block
      uint64_t message_block = *reinterpret_cast<const uint64_t*>(aligned_data + i);
      std::cout << "message block " << message_block << std::endl;

      // Mix the message block into the state by XOR
      stateR += (message_block + 1 + i + len);
      //stateS ^= message_block;

      // Transform the state by multiplying by a generator modulo the prime
      stateR = (stateR * C) % R;
      //stateS = (stateS * D) % S;
    }

    // Handle the remaining bytes (if the message length is not a multiple of 8)
    uint64_t final_block = 0;
    for (int j = i; j < len; ++j) {
      std::cout << "j " << j << std::endl;
      final_block <<= 8;
      final_block |= aligned_data[j];
    }
    if (len % 8 != 0) {
      std::cout << "final block " << final_block << std::endl;
      stateR += (1 + len + final_block);
      //stateS ^= final_block;
      stateR = (stateR * C) % R;
      //stateS = (stateS * D) % S;
    }

    // Finalization step: combine the states and write the result to the output buffer
    hash_value[0] = stateR;

    // Example usage of std::cout to print a message
    //std::cout << "Hash computation completed!" << std::endl;
  }
}

