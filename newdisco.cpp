#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

constexpr uint64_t P = UINT64_C(1764231181088413613);
constexpr uint64_t A = UINT64_C(1052833812310576281);

constexpr uint64_t Q = UINT64_C(1596490590891650011);
constexpr uint64_t B = UINT64_C(248738986944023899);

static inline void process_chunk(uint64_t& hash, uint64_t k) {
    // Core transformation
    k = (k * A) % P;

    // Mix the transformation into the accumulated hash value
    hash ^= k;
    hash = (hash << 1) | (hash >> 63); // Rotate left by 1
}

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key);
    uint64_t hash = seed;
    uint64_t k;

    // Process each 64-bit chunk of the input data
    for (int i = 0; i < len / 8; ++i) {
        memcpy(&k, data + i * 8, 8);
        process_chunk(hash, k);
    }

    // Process remaining bytes if len is not a multiple of 8
    if (len % 8) {
        k = 0;
        memcpy(&k, data + len / 8 * 8, len % 8);
        process_chunk(hash, k);
    }

    // Final mixing
    hash ^= len;
    hash ^= (hash >> 33);
    hash *= 0xff51afd7ed558ccd;
    hash ^= (hash >> 33);
    hash *= 0xc4ceb9fe1a85ec53;
    hash ^= (hash >> 33);

    // Copy the result to the output buffer
    memcpy(out, &hash, sizeof(hash));
}

