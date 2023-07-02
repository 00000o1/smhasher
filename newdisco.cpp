#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <omp.h> // For OpenMP

const uint64_t PRIME = 13166748625691186689ULL;
const uint64_t GENERATOR = 3180491373ULL;

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
  // Get and align the input data to 8-byte boundaries
  const uint8_t* data = reinterpret_cast<const uint8_t*>(key);
  alignas(uint64_t) uint8_t aligned_data[len];
  memcpy(aligned_data, data, len);
  const uint64_t* data64 = reinterpret_cast<const uint64_t*>(aligned_data);

  uint64_t state = len;
  state -= seed;
  state = state << 32 | (seed + len);

  // Processing 64-bit blocks
  for (int i = 0; i < len / 8; i++) {
      state -= static_cast<uint64_t>(data64[i]);
      state *= GENERATOR;
      state %= PRIME;
  }

  // Processing remaining bytes
  uint64_t remainder = 0;
  int offset = (len / 8) * 8;
  for (int i = 0; i < len - offset; i++) {
      remainder |= static_cast<uint64_t>(aligned_data[offset + i]) << (8 * i);
  }
  if (len - offset > 0) {
      state -= remainder;
      state *= GENERATOR;
      state %= PRIME;
  }

  memcpy(out, &state, sizeof(state));
}

