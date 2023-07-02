#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <omp.h> // For OpenMP

constexpr uint64_t PRIME_MULTIPLIER = 0xFFFFFFFFFFFFFFFF - 58;
constexpr uint64_t SEED = 0x9747b28c;

inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

inline uint64_t fmix64(uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
}

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
  // fast hash that hashes key of length len including seed to produce 64-bit output to out

  // Get and align the input data to 8-byte boundaries
  const uint8_t* data = reinterpret_cast<const uint8_t*>(key);
  alignas(uint64_t) uint8_t aligned_data[len];
  memcpy(aligned_data, data, len);
  const uint64_t* data64 = reinterpret_cast<const uint64_t*>(aligned_data);

  uint64_t hash = SEED;

  // parallelize for inputs above a certain size
  if (len >= 256) {
    #pragma omp parallel for reduction(^: hash)
    for (int i = 0; i < len / 8; i++) {
      uint64_t k = data64[i];
      k *= PRIME_MULTIPLIER;
      k ^= k >> 47;
      k *= PRIME_MULTIPLIER;
      hash ^= k;
    }
  } else {
    for (int i = 0; i < len / 8; i++) {
      uint64_t k = data64[i];
      k *= PRIME_MULTIPLIER;
      k ^= k >> 47;
      k *= PRIME_MULTIPLIER;
      hash ^= k;
    }
  }

  // deal with non-block sized input left over
  uint64_t remainder_hash = 0;
  int offset = (len / 8) * 8;
  for (int i = 0; i < len - offset; i++) {
    remainder_hash = remainder_hash | (aligned_data[offset + i] << (8 * i));
  }
  remainder_hash *= PRIME_MULTIPLIER;
  remainder_hash ^= remainder_hash >> 47;
  remainder_hash *= PRIME_MULTIPLIER;
  hash ^= remainder_hash;

  // finalize
  hash ^= len;
  hash = fmix64(hash);

  memcpy(out, &hash, sizeof(hash));
}

