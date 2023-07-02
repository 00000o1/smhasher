#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <omp.h> // For OpenMP

constexpr uint64_t PRIME_MULTIPLIER = 0xFFFFFFFFFFFFFFFF - 58;

inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
  // fast hash that hashes key of length len including seed to produce 64-bit output to out

  // Get and align the input data to 8-byte boundaries
  const uint8_t* data = reinterpret_cast<const uint8_t*>(key);
  alignas(uint64_t) uint8_t aligned_data[len];
  memcpy(aligned_data, data, len);
  const uint64_t* data64 = reinterpret_cast<const uint64_t*>(aligned_data);

  uint64_t hash = 0;

  // parallelize for inputs above a certain size
  if (len >= 256) {
    #pragma omp parallel for reduction(^: hash)
    for (int i = 0; i < len / 8; i++) {
      // process data64
      // ...
    }
  } else {
    for (int i = 0; i < len / 8; i++) {
      // process data64
      // ...
    }
  }

  // deal with non-block sized input left over
  uint64_t remainder_hash = 0;
  int offset = (len / 8) * 8;
  for (int i = 0; i < len - offset; i++) {
    // process aligned_data[offset + i]
    // ...
  }

  // finalize
  // ...


  memcpy(out, &hash, sizeof(hash));
}

