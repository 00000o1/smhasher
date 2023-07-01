#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>

constexpr uint64_t PRIME_MULTIPLIER = 0x880355f21e6d1965ULL;

inline uint64_t rotl64(uint64_t x, int8_t r) {
    return (x << r) | (x >> (64 - r));
}

void process_chunk(const uint8_t* data, int len, unsigned seed, uint64_t& intermediate_hash) {
    uint64_t hash = seed + PRIME_MULTIPLIER;

    int num_blocks = len / 8;
    for (int i = 0; i < num_blocks; i++) {
        uint64_t block;
        memcpy(&block, data + i * 8, 8);

        hash ^= block;
        hash = rotl64(hash, 13);
        hash = hash * PRIME_MULTIPLIER + 0x9E3779B97F4A7C15ULL;
    }

    int offset = num_blocks * 8;
    for (int i = 0; i < len - offset; i++) {
        hash ^= data[offset + i];
        hash = rotl64(hash, 5);
        hash = hash * PRIME_MULTIPLIER + 0xBF58476D1CE4E5B9ULL;
    }

    intermediate_hash ^= hash;
}

void newdisco_64(const void* key, int len, unsigned seed, void* out) {
    const uint8_t* original_data = reinterpret_cast<const uint8_t*>(key);

    // Allocate and align input data
    alignas(uint64_t) uint8_t aligned_data[len];
    memcpy(aligned_data, original_data, len);
    const uint8_t* data = aligned_data;

    unsigned int num_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(num_threads);

    // Align intermediate hashes
    alignas(16) std::vector<uint64_t> intermediate_hashes(num_threads, 0);

    int chunk_size = (len + num_threads - 1) / num_threads;

    for (unsigned int i = 0; i < num_threads; ++i) {
        int chunk_start = i * chunk_size;
        int chunk_len = std::min(chunk_size, len - chunk_start);
        if (chunk_len <= 0) break;

        threads[i] = std::thread(process_chunk, data + chunk_start, chunk_len, seed, std::ref(intermediate_hashes[i]));
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    uint64_t hash = seed + PRIME_MULTIPLIER;
    for (auto interm_hash : intermediate_hashes) {
        hash ^= interm_hash;
        hash = rotl64(hash, 13);
        hash = hash * PRIME_MULTIPLIER + 0x9E3779B97F4A7C15ULL;
    }

    // Finalization
    hash ^= len;
    hash ^= (hash >> 33);
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= (hash >> 33);
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= (hash >> 33);

    memcpy(out, &hash, sizeof(hash));
}


