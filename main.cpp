#define _MAIN_CPP
#include "Platform.h"
#include "Hashes.h"
#include "KeysetTest.h"
#include "SpeedTest.h"
#include "AvalancheTest.h"
#include "DifferentialTest.h"
#include "HashMapTest.h"

#if NCPU > 1 // disable with -DNCPU=0 or 1
#include <thread>
#include <chrono>
#endif

#include <stdio.h>
#include <stdint.h>
#include <time.h>

//-----------------------------------------------------------------------------
// Configuration.

bool g_drawDiagram     = false;
bool g_testAll         = true;
bool g_testExtra       = false; // excessive torture tests: Sparse, Avalanche, DiffDist, scan all seeds
bool g_testVerifyAll   = false;

bool g_testSanity      = false;
bool g_testSpeed       = false;
bool g_testHashmap     = false;
bool g_testAvalanche   = false;
bool g_testSparse      = false;
bool g_testPermutation = false;
bool g_testWindow      = false;
bool g_testCyclic      = false;
bool g_testTwoBytes    = false;
bool g_testText        = false;
bool g_testZeroes      = false;
bool g_testSeed        = false;
bool g_testPerlinNoise = false;
bool g_testDiff        = false;
bool g_testDiffDist    = false;
bool g_testMomentChi2  = false;
bool g_testPrng        = false;
bool g_testBIC         = false;
bool g_testBadSeeds    = false;
//bool g_testLongNeighbors = false;

double g_speed = 0.0;

struct TestOpts {
bool         &var;
const char*  name;
};
TestOpts g_testopts[] =
{
{ g_testAll,          "All" },
{ g_testVerifyAll,    "VerifyAll" },
{ g_testSanity,       "Sanity" },
{ g_testSpeed,        "Speed" },
{ g_testHashmap,      "Hashmap" },
{ g_testAvalanche,    "Avalanche" },
{ g_testSparse,       "Sparse" },
{ g_testPermutation,  "Permutation" },
{ g_testWindow,       "Window" },
{ g_testCyclic,       "Cyclic" },
{ g_testTwoBytes,     "TwoBytes" },
{ g_testText,	        "Text" },
{ g_testZeroes,       "Zeroes" },
{ g_testSeed,	        "Seed" },
{ g_testPerlinNoise,	"PerlinNoise" },
{ g_testDiff,         "Diff" },
{ g_testDiffDist,     "DiffDist" },
{ g_testBIC, 	        "BIC" },
{ g_testMomentChi2,   "MomentChi2" },
{ g_testPrng,         "Prng" },
{ g_testBadSeeds,     "BadSeeds" },
//{ g_testLongNeighbors,"LongNeighbors" }
};

bool MomentChi2Test ( struct HashInfo *info, int inputSize );

//-----------------------------------------------------------------------------
// This is the list of all hashes that SMHasher can test.

const char* quality_str[3] = { "SKIP", "POOR", "GOOD" };

// sorted by quality and speed. the last is the list of internal secrets to be tested against bad seeds.
// marked with !! are known bad seeds, which either hash to 0 or create collisions.
HashInfo g_hashes[] =
{
// first the bad hash funcs, failing tests:
{ DoNothingHash,        32, 0x0, "donothing32", "Do-Nothing function (measure call overhead)", SKIP, {0UL} /* !! */ },
{ DoNothingHash,        64, 0x0, "donothing64", "Do-Nothing function (measure call overhead)", SKIP, {0ULL} /* !! */ },
{ DoNothingHash,       128, 0x0, "donothing128", "Do-Nothing function (measure call overhead)", SKIP, {0UL} /* !! */ },
{ NoopOAATReadHash,     64, 0x0, "NOP_OAAT_read64", "Noop function (measure call + OAAT reading overhead)", SKIP, {} },
{ BadHash,     	  32, 0xAB432E23, "BadHash", 	 "very simple XOR shift", SKIP, {0UL} /* !! */ },
{ sumhash,     	  32, 0x0000A9AC, "sumhash", 	 "sum all bytes", SKIP, {0UL} /* !! */ },
{ sumhash32,     	  32, 0x3D6DC280, "sumhash32",   "sum all 32bit words", SKIP, {0x9e3779b97f4a7c15} },

// here start the real hashes. first the problematic ones:
#ifdef HAVE_BIT32
#define FIBONACCI_VERIF      0x09952480
#define FNV2_VERIF           0x739801C5
#else
#define FIBONACCI_VERIF      0xFE3BD380
#define FNV2_VERIF           0x1967C625
#endif

#ifdef __SIZEOF_INT128__
// M. Dietzfelbinger, T. Hagerup, J. Katajainen, and M. Penttonen. A reliable randomized
// algorithm for the closest-pair problem. J. Algorithms, 25:19–51, 1997.
{ multiply_shift,       64, 0xFCE355A6, "multiply_shift", "Dietzfelbinger Multiply-shift on strings", POOR,
  { 0xfffffff0, 0x1fffffff0, 0xb13dea7c9c324e51ULL, 0x75f17d6b3588f843ULL } /* !! all & 0xfffffff0 (2^32 bad seeds) */ },
{ pair_multiply_shift,  64, 0xD4B20347, "pair_multiply_shift", "Pair-multiply-shift", POOR,
  { 0xb13dea7c9c324e51ULL, 0x75f17d6b3588f843ULL } },
#endif
{ crc32,                32, 0x3719DB20, "crc32",       "CRC-32 soft", POOR, {} },
{ md5_128,             128, 0xF263F96F, "md5-128",     "MD5", GOOD, {} },
{ md5_64,               64, 0x12F0BA8E, "md5_64",      "MD5, bits 32-95", GOOD, {} },
{ md5_32,               32, 0xF3DFF19F, "md5_32",      "MD5, bits 32-63", GOOD, {} },
#ifdef _MSC_VER /* truncated long to 32 */
#  define SHA1_VERIF            0xED2F35E4
#  define SHA1_32_VERIF         0x00000000
#  define SHA1_64_VERIF         0x00000000
#else
#  define SHA1_VERIF            0x6AF411D8
#  define SHA1_32_VERIF         0xB3122757
#  define SHA1_64_VERIF         0x995397D5
#endif
{ sha1_160,            160, SHA1_VERIF, "sha1-160",     "SHA1", GOOD},
{ sha1_32,              32, SHA1_32_VERIF,"sha1_32",    "SHA1, low 32 bits", GOOD},
{ sha1_64,              64, SHA1_64_VERIF,"sha1_64",    "SHA1, low 64 bits", GOOD},
{ sha2_224,            224, 0x407AA518, "sha2-224",     "SHA2-224", POOR, {} },
{ sha2_224_64,          64, 0xF3E40ECA, "sha2-224_64",  "SHA2-224, low 64 bits", POOR, {} },
{ sha2_256,            256, 0xEBDA2FB1, "sha2-256",     "SHA2-256", GOOD, {} },
{ sha2_256_64,          64, 0xC1C4FA72, "sha2-256_64",  "SHA2-256, low 64 bits", GOOD, {} },
#if defined(HAVE_SHANI) && defined(__x86_64__)
{ sha1ni,              160, 0x375755A4, "sha1ni",       "SHA1_NI (amd64 HW SHA ext)", POOR, {0x67452301} },
{ sha1ni_32,            32, 0xE70686CC, "sha1ni_32",    "hardened SHA1_NI (amd64 HW SHA ext), low 32 bits", GOOD,
  {0x67452301} },
{ sha2ni_256,          256, 0x4E3BB25E, "sha2ni-256",   "SHA2_NI-256 (amd64 HW SHA ext)", POOR, {0x6a09e667} },
{ sha2ni_256_64,        64, 0xF938E80E, "sha2ni-256_64","hardened SHA2_NI-256 (amd64 HW SHA ext), low 64 bits", POOR, {0x6a09e667} },
#endif
{ rmd128,              128, 0xFF576977, "rmd128",       "RIPEMD-128", GOOD, {} },
{ rmd160,              160, 0x30B37AC6, "rmd160",       "RIPEMD-160", GOOD, {} },
{ rmd256,              256, 0xEB16FAD7, "rmd256",       "RIPEMD-256", GOOD, {} },
{ edonr224,            224, 0x83A8E7AB, "edonr224",     "EDON-R 224", GOOD, {} },
{ edonr256,            256, 0x06DD4F96, "edonr256",     "EDON-R 256", GOOD, {} },
#if defined(HAVE_BIT32) && !defined(_WIN32)
#  define BLAKE3_VERIF   0x58571F56
#else
#  define BLAKE3_VERIF   0x50E4CD91
#endif
{ blake3c_test,        256, BLAKE3_VERIF, "blake3_c",   "BLAKE3 c",   GOOD, {0x6a09e667} },
#if defined(HAVE_BLAKE3)
{ blake3_test,         256, 0x0, "blake3",       "BLAKE3 Rust", GOOD, {} },
{ blake3_64,            64, 0x0, "blake3_64",    "BLAKE3 Rust, low 64 bits", GOOD, {} },
#endif
{ blake2s128_test,     128, 0xE8D8FCDF, "blake2s-128",  "blake2s-128", GOOD, {} },
{ blake2s160_test,     160, 0xD50FF144, "blake2s-160",  "blake2s-160", GOOD, {} },
{ blake2s224_test,     224, 0x19B36D2C, "blake2s-224",  "blake2s-224", GOOD, {} },
{ blake2s256_test,     256, 0x841D6354, "blake2s-256",  "blake2s-256", GOOD, {} },
{ blake2s256_64,        64, 0x53000BB2, "blake2s-256_64","blake2s-256, low 64 bits", GOOD, {} },
{ blake2b160_test,     160, 0x28ADDA30, "blake2b-160",  "blake2b-160", GOOD, {} },
{ blake2b224_test,     224, 0x101A62A4, "blake2b-224",  "blake2b-224", GOOD, {} },
{ blake2b256_test,     256, 0xC9D8D995, "blake2b-256",  "blake2b-256", GOOD, {} },
{ blake2b256_64,        64, 0xCF4F7EC3, "blake2b-256_64","blake2b-256, low 64 bits", GOOD, {} },
{ asconhashv12_256,    256, 0xA969C160, "asconhashv12", "asconhashv12 256bit", GOOD,
  { 0xee9398aadb67f03dULL } },
{ asconhashv12_64,      64, 0xE7DEF300, "asconhashv12_64", "asconhashv12, low 64 bits", GOOD,
  { 0xee9398aadb67f03dULL } },
{ sha3_256,            256, 0x21048CE3, "sha3-256",     "SHA3-256 (Keccak)", POOR, {} },
{ sha3_256_64,          64, 0xE62E5CC0, "sha3-256_64",  "SHA3-256 (Keccak), low 64 bits", POOR, {} },
#if defined(HAVE_SSE2)
{ hasshe2_test,        256, 0xF5D39DFE, "hasshe2",     "SSE2 hasshe2, 256-bit", POOR, {} },
#endif
// too fragile
#ifdef __SIZEOF_INT128__
#ifdef __FreeBSD__
#  define POLY1_VERIF   0xFECCC371
#  define POLY2_VERIF   0xA31DD921
#  define POLY3_VERIF   0x26F7EDA0
#  define POLY4_VERIF   0x8EE270BD
#  define TABUL_VERIF   0x4917178C
#elif defined __apple_build_version__ && defined __clang__
#  define POLY1_VERIF   0xE389931F
#  define POLY2_VERIF   0x458D056D
#  define POLY3_VERIF   0x208145CC
#  define POLY4_VERIF   0xE798712E
#  define TABUL_VERIF   0x91618263
#elif defined DEBUG
#  define POLY1_VERIF   0x9E4BA93D
#  define POLY2_VERIF   0x5CB4CB25
#  define POLY3_VERIF   0x3C87C852
#  define POLY4_VERIF   0xFF88BAF6
#  define TABUL_VERIF   0xB49C607C
#else
#  define POLY1_VERIF   0x64706572
#  define POLY2_VERIF   0xE8906EDF
#  define POLY3_VERIF   0xF2A7E178
#  define POLY4_VERIF   0xD4E89421
#  define TABUL_VERIF   0xB49C607C
#endif
// Thomas Dybdahl Ahle, Jakob Tejs Bæk Knudsen, and Mikkel Thorup
// "The Power of Hashing with Mersenne Primes".
{ poly_1_mersenne,      32, 0, "poly_1_mersenne", "Degree 1 Hashing mod 2^61-1", POOR, {} },
{ poly_2_mersenne,      32, 0, "poly_2_mersenne", "Degree 2 Hashing mod 2^61-1", GOOD, {0x60e8512c} /* !! */},
{ poly_3_mersenne,      32, 0, "poly_3_mersenne", "Degree 3 Hashing mod 2^61-1", GOOD, {0x3d25f745} /* !! */},
{ poly_4_mersenne,      32, 0, "poly_4_mersenne", "Degree 4 Hashing mod 2^61-1", GOOD, {} },
{ tabulation_test,      64, TABUL_VERIF, "tabulation",      "64-bit Tabulation with Multiply-Shift Mixer", GOOD, {} },
#endif
#if defined(_MSC_VER) /* truncated long to 32 */
#  define TABUL32_VERIF   0x3C3B7BDD
#elif defined __FreeBSD__
#  define TABUL32_VERIF   0x84F58D1F
#elif defined __apple_build_version__ && defined __clang__
#  define TABUL32_VERIF   0x2C8EDFFE
#else
#  define TABUL32_VERIF   0x335F64EA
#endif
{ tabulation_32_test,   32, TABUL32_VERIF, "tabulation32",    "32-bit Tabulation with Multiply-Shift Mixer", POOR, {} },
#ifdef HAVE_SSE42
// all CRC's are insecure by default due to its polynomial nature.
/* Even 32 uses crc32q, quad only */
#  if defined(_MSC_VER) /* truncated long to 32? */
#   define CRC32_VERIF   0xC2B84071
#   define CRC64_VERIF   0x6BBC19D6
#  else  
#   define CRC32_VERIF   0x0C7346F0
#   define CRC64_VERIF   0xE7C3FD0E
#  endif
# ifndef HAVE_BROKEN_MSVC_CRC32C_HW
{ crc32c_hw_test,       32, CRC32_VERIF, "crc32_hw",    "SSE4.2 crc32 in HW", POOR, {0x111c2232} /* !! */},
{ crc64c_hw_test,       64, CRC64_VERIF, "crc64_hw",    "SSE4.2 crc64 in HW", POOR, {0x0} /* !! */ },
# endif
# if defined(__SSE4_2__) && (defined(__i686__) || defined(__x86_64__)) && !defined(_MSC_VER)
{ crc32c_hw1_test,      32, 0x0C7346F0,  "crc32_hw1",   "Faster Adler SSE4.2 crc32 on Intel HW", POOR, {0x111c2232} /* !! */},
# endif
#endif
// 32bit crashes
#if defined(HAVE_CLMUL) && !defined(_MSC_VER) && defined(__x86_64__)
{ crc32c_pclmul_test,   32, 0x0, "crc32_pclmul","-mpclmul crc32 in asm on HW", POOR, {0x0} /* !! */ },
#endif
#ifdef HAVE_INT64
{ o1hash_test,          64, 0x85051E87, "o1hash",       "o(1)hash unseeded, from wyhash", POOR, {0x0} /* !! */ },
#endif
#if 0 && defined(__x86_64__) && (defined(__linux__) || defined(__APPLE__))
// elf64 or macho64 only
{ fhtw_test,            64, 0x0,        "fhtw",        "fhtw asm", POOR, {} },
#endif
{ fibonacci_test, __WORDSIZE, FIBONACCI_VERIF, "fibonacci",   "wordwise Fibonacci", POOR,
  {0x0, 0xffffffff00000000ULL} /* !! all keys ending with 0x0000_0000 */ },
#ifndef HAVE_ALIGNED_ACCESS_REQUIRED
{ khash32_test,         32, 0x99B3FFCD, "k-hash32",    "K-Hash mixer, 32-bit", POOR, {} },
{ khash64_test,         64, 0xAB5518A1, "k-hash64",    "K-Hash mixer, 64-bit", POOR, {} },
#endif
{ FNV32a_test,          32, 0xE3CBBE91, "FNV1a",       "Fowler-Noll-Vo hash, 32-bit", POOR,
  {0x811c9dc5} /* !! */ },
#ifdef HAVE_INT64
{ FNV1A_Totenschiff_test,32,0x95D95ACF, "FNV1A_Totenschiff",  "FNV1A_Totenschiff_v1 64-bit sanmayce", POOR,
  {0x811c9dc5} },
{ FNV1A_PY_test,        32, 0xE79AE3E4, "FNV1A_Pippip_Yurii", "FNV1A-Pippip_Yurii 32-bit sanmayce", POOR,
  {0x811c9dc5} },
{ FNV32a_YT_test,       32, 0xD8AFFD71, "FNV1a_YT",    "FNV1a-YoshimitsuTRIAD 32-bit sanmayce", POOR,
  {0x811c9dc5, 0x23d4a49d} /* !! */ },
{ FNV64a_test,          64, 0x103455FC, "FNV64",       "Fowler-Noll-Vo hash, 64-bit", POOR,
  {0x811c9dc5, 0xcbf29ce4, 0x84222325, 0xcbf29ce484222325} /* TODO */},
{ FNV128_test,         128, 0xBCAA1426, "FNV128",      "Go variant of FNV, 128-bit", POOR, {} },
#endif
{ FNV2_test,    __WORDSIZE, FNV2_VERIF, "FNV2",        "wordwise FNV", POOR, {} },
{ fletcher2_test,       64, 0x890767C0, "fletcher2",   "fletcher2 ZFS", POOR, {0UL} /* !! */ },
{ fletcher4_test,       64, 0x890767C0, "fletcher4",   "fletcher4 ZFS", POOR, {0UL} /* !! */ },
{ Bernstein_test,       32, 0xBDB4B640, "bernstein",   "Bernstein, 32-bit", POOR, {0UL} /* !! */ },
{ sdbm_test,            32, 0x582AF769, "sdbm",        "sdbm as in perl5", POOR, {0UL} /* !! */ },
{ x17_test,             32, 0x8128E14C, "x17",         "x17", POOR, {} },
{ libiberty_test,       32, 0x584FBC20, "libiberty",   "libiberty htab_hash_string", POOR, {0x2ba97ba0} },
{ gcc_test,             32, 0xC6239327, "gcc",         "gcc libcpp", POOR, {0xbaa0af90} },
// also called jhash:
{ JenkinsOOAT_test,     32, 0x83E133DA, "JenkinsOOAT", "Bob Jenkins' OOAT as in perl 5.18", POOR, {0UL} /* !! */ },
{ JenkinsOOAT_perl_test,32, 0xEE05869B, "JenkinsOOAT_perl", "Bob Jenkins' OOAT as in old perl5", POOR, {0UL} /* !! */},
#if defined(HAVE_SSE42) && defined(__x86_64__) && !defined(_MSC_VER)
// empty verify with msvc. avoid
{ pearson64_test,       64, 0x12E4C8CD, "pearsonhash64",    "Pearson hash, 64-bit SSSE3", POOR, {}},
{ pearson128_test,     128, 0x6CCBB7B3, "pearsonhash128",   "Pearson hash, 128-bit SSSE3, low 64-bit", POOR, {}},
{ pearson256_test,     256, 0x7F8BEB21, "pearsonhash256",   "Pearson hash, 256-bit SSSE3, low 64-bit", POOR, {}},
#endif
#ifdef HAVE_INT64
{ pearsonb64_test,      64, 0xB6FF2DFC, "pearsonbhash64",  "Pearson block hash, 64-bit", GOOD, {}},
{ pearsonb128_test,    128, 0x6BEFE6EA, "pearsonbhash128", "Pearson block hash, 128-bit, low 64-bit", GOOD, {}},
{ pearsonb256_test,    256, 0x999B3C19, "pearsonbhash256", "Pearson block hash, 256-bit, low 64-bit", GOOD, {}},
#endif
// FIXME: seed
#ifdef __aarch64__
#define VHASH32_VERIF 0x0F02AEFD
#define VHASH64_VERIF 0xFAAEE597
#else
#define VHASH32_VERIF 0xF0077651
#define VHASH64_VERIF 0xF97D84FE
#endif
{ VHASH_32,             32, VHASH32_VERIF, "VHASH_32",    "VHASH_32 by Ted Krovetz and Wei Dai", POOR, {} },
{ VHASH_64,             64, VHASH64_VERIF, "VHASH_64",    "VHASH_64 by Ted Krovetz and Wei Dai", POOR, {} },
{ MicroOAAT_test,       32, 0x16F1BA97,    "MicroOAAT",   "Small non-multiplicative OAAT (by funny-falcon)", POOR,
  {0x3b00} },
#ifdef HAVE_SSE2
{ farsh32_test,         32, 0xBCDE332C, "farsh32",     "FARSH 32bit", POOR, {} }, // insecure
{ farsh64_test,         64, 0xDE2FDAEE, "farsh64",     "FARSH 64bit", POOR, {} }, // insecure
{ farsh128_test,       128, 0x82B6CBEC, "farsh128",    "FARSH 128bit", POOR, {} },
{ farsh256_test,       256, 0xFEBEA0BC, "farsh256",    "FARSH 256bit", POOR, {} },
#endif
{ jodyhash32_test,      32, 0xA2AEFC60, "jodyhash32",  "jodyhash, 32-bit (v6)", POOR, {0x9fd368f4} /* !! */ },
#ifdef HAVE_INT64
{ jodyhash64_test,      64, 0xC1CBFA34, "jodyhash64",  "jodyhash, 64-bit (v7.1)", POOR, {} },
#endif
{ lookup3_test,         32, 0x3D83917A, "lookup3",     "Bob Jenkins' lookup3", POOR, {0x21524101} /* !! */},
#ifdef __aarch64__
#define SFAST_VERIF 0x6306A6FE
#else
#define SFAST_VERIF 0x0C80403A
#endif
{ SuperFastHash_test,   32, SFAST_VERIF,"superfast",   "Paul Hsieh's SuperFastHash", POOR, {0x0} /* !! */},
{ MurmurOAAT_test,      32, 0x5363BD98, "MurmurOAAT",  "Murmur one-at-a-time", POOR,
  {0x0 /*, 0x5bd1e995*/} /* !! */ },
{ Crap8_test,           32, 0x743E97A1, "Crap8",       "Crap8", POOR, {/*0x83d2e73b, 0x97e1cc59*/} },
{ xxHash32_test,        32, 0xBA88B743, "xxHash32",    "xxHash, 32-bit for x86", POOR, {} },
{ MurmurHash1_test,     32, 0x9EA7D056, "Murmur1",     "MurmurHash1", POOR, {0xc6a4a793} /* !! */ },
{ MurmurHash2_test,     32, 0x27864C1E, "Murmur2",     "MurmurHash2 for x86, 32-bit", POOR,
  {0x10} /* !! */ },
{ MurmurHash2A_test,    32, 0x7FBD4396, "Murmur2A",    "MurmurHash2A for x86, 32-bit", POOR,
  {0x2fc301c9} /* !! */ },
#if __WORDSIZE >= 64
{ MurmurHash64A_test,   64, 0x1F0D3804, "Murmur2B",    "MurmurHash64A for x64, 64-bit", POOR,
  {0xc6a4a7935bd1e995ULL} },
#endif
#ifdef HAVE_INT64
{ MurmurHash64B_test,   64, 0xDD537C05, "Murmur2C",    "MurmurHash64B for x86, 64-bit", POOR,
  {0x10, 0xffffffff00000010 } /* !! *00000010 */ },
#endif
{ MurmurHash3_x86_32,   32, 0xB0F57EE3, "Murmur3A",    "MurmurHash3 for x86, 32-bit", POOR,
  {0xfca58b2d} /* !! */},
{ PMurHash32_test,      32, 0xB0F57EE3, "PMurHash32",  "Shane Day's portable-ized MurmurHash3 for x86, 32-bit", POOR,
  {0xfca58b2d} /* !! */ }, // 0x4b600, 0xcc9e2d51
{ MurmurHash3_x86_128, 128, 0xB3ECE62A, "Murmur3C",    "MurmurHash3 for x86, 128-bit", POOR, {0x239b961b} },
#if !defined(DEBUG) && !defined(CROSSCOMPILING) && !defined(__aarch64__)
# ifndef HAVE_ASAN
// TODO seeded
{ PMPML_32_CPP,         32, 0xEAE2E3CC, "PMPML_32",    "PMP_Multilinear 32-bit unseeded", POOR, {} },
#  if defined(_WIN64) || defined(__x86_64__)
{ PMPML_64_CPP,         64, 0x584CC9DF, "PMPML_64",    "PMP_Multilinear 64-bit unseeded", POOR, {} },
#  endif
# endif
#endif
{ CityHash32_test,      32, 0x5C28AD62, "City32",      "Google CityHash32WithSeed (old)", POOR, {0x2eb38c9f} /* !! */},
#ifdef HAVE_INT64
{ metrohash64_test,      64, 0x6FA828C9, "metrohash64",    "MetroHash64, 64-bit", POOR, {} },
{ metrohash64_1_test,    64, 0xEE88F7D2, "metrohash64_1",  "MetroHash64_1, 64-bit (legacy)", POOR, {} },
{ metrohash64_2_test,    64, 0xE1FC7C6E, "metrohash64_2",  "MetroHash64_2, 64-bit (legacy)", GOOD, {} },
{ metrohash128_test,    128, 0x4A6673E7, "metrohash128",   "MetroHash128, 128-bit", GOOD, {} },
{ metrohash128_1_test,  128, 0x20E8A1D7, "metrohash128_1", "MetroHash128_1, 128-bit (legacy)", GOOD, {} },
{ metrohash128_2_test,  128, 0x5437C684, "metrohash128_2", "MetroHash128_2, 128-bit (legacy)", GOOD, {} },
#endif
#if defined(HAVE_SSE42) && (defined(__x86_64__) ||  defined(__aarch64__)) && !defined(_MSC_VER)
{ metrohash64crc_1_test, 64, 0x29C68A50, "metrohash64crc_1", "MetroHash64crc_1 for x64 (legacy)", POOR, {} },
{ metrohash64crc_2_test, 64, 0x2C00BD9F, "metrohash64crc_2", "MetroHash64crc_2 for x64 (legacy)", POOR, {} },
{ cmetrohash64_1_optshort_test,64, 0xEE88F7D2, "cmetrohash64_1o", "cmetrohash64_1 (shorter key optimized), 64-bit for x64", POOR, {} },
{ cmetrohash64_1_test,   64, 0xEE88F7D2, "cmetrohash64_1",  "cmetrohash64_1, 64-bit for x64", POOR, {} },
{ cmetrohash64_2_test,   64, 0xE1FC7C6E, "cmetrohash64_2",  "cmetrohash64_2, 64-bit for x64", GOOD, {} },
{ metrohash128crc_1_test,128, 0x5E75144E, "metrohash128crc_1", "MetroHash128crc_1 for x64 (legacy)", GOOD, {} },
{ metrohash128crc_2_test,128, 0x1ACF3E77, "metrohash128crc_2", "MetroHash128crc_2 for x64 (legacy)", GOOD, {} },
#endif
{ CityHash64noSeed_test, 64, 0x63FC6063, "City64noSeed","Google CityHash64 without seed (default version, misses one final avalanche)", POOR, {} },
{ CityHash64_test,      64, 0x25A20825, "City64",       "Google CityHash64WithSeed (old)", POOR, {} },
#if defined(HAVE_SSE2) && defined(HAVE_AESNI) && !defined(_MSC_VER)
{ aesnihash_test,       64, 0xA68E0D42, "aesnihash",    "majek's seeded aesnihash with aesenc, 64-bit for x64", POOR,
  {0x70736575} },

{ aesni128_test,       128, 0xF06DA1B1, "aesni",    "aesni 128bit", GOOD,{} },
{ aesni64_test,         64, 0x3AA1A480, "aesni-low","aesni 64bit",  GOOD,{} },
#endif
#if defined(HAVE_SSE2) && defined(__x86_64__) && !defined(_WIN32) && !defined(_MSC_VER)
{ falkhash_test_cxx,    64, 0x2F99B071, "falkhash",    "falkhash.asm with aesenc, 64-bit for x64", POOR, {} },
#endif
#ifdef HAVE_MEOW_HASH
{ MeowHash32_test,      32, 0x8872DE1A, "MeowHash32low","MeowHash (requires x64 AES-NI)", POOR,
  {0x920e7c64} /* !! */},
{ MeowHash64_test,      64, 0xB04AC842, "MeowHash64low","MeowHash (requires x64 AES-NI)", POOR, {0x920e7c64} },
{ MeowHash128_test,    128, 0xA0D29861, "MeowHash",     "MeowHash (requires x64 AES-NI)", POOR, {0x920e7c64} },
#endif
{ t1ha1_64le_test,      64, 0xD6836381, "t1ha1_64le",  "Fast Positive Hash (portable, aims 64-bit, little-endian)", POOR, {} },
{ t1ha1_64be_test,      64, 0x93F864DE, "t1ha1_64be",  "Fast Positive Hash (portable, aims 64-bit, big-endian)", POOR, {} },
{ t1ha0_32le_test,      64, 0x7F7D7B29, "t1ha0_32le",  "Fast Positive Hash (portable, aims 32-bit, little-endian)", POOR, {} },
{ t1ha0_32be_test,      64, 0xDA6A4061, "t1ha0_32be",  "Fast Positive Hash (portable, aims 32-bit, big-endian)", POOR, {} },
#if __WORDSIZE >= 64
# define TIFU_VERIF       0x644236D4
#else
// broken on certain travis
# define TIFU_VERIF       0x0
#endif
// and now the quality hash funcs, slowest first
{ tifuhash_64,          64, TIFU_VERIF, "tifuhash_64", "Tiny Floatingpoint Unique Hash with continued egyptian fractions", GOOD, {} },
{ beamsplitter_64,      64, 0x1BDF358B, "beamsplitter","A possibly universal hash made with a 10x64 s-box.", GOOD, {} },
{ discohash1::DISCoHAsH<64>,        64, 0xBEBB4185, "discohash1",   "DISCoHAsH 64 (was BEBB4185) v1", GOOD,
  {
    0x107a36, 0x110890, 0x124ac7, 0x125ce5, 0x126314, 0x126ce, 0x127b0, 0x1296ec,
    0x132da3, 0x133490, 0x133d5d, 0x1375b7, 0x1377a1, 0x137a87, 0x13becb,
    0x148e56, 0x14d1f, 0x15587c, 0x15e0ba, 0x162cb7, 0x1690a6, 0x16ccf,
    0x16d4a, 0x16d4b0, 0x1752fa, 0x1a675b, 0x1a7cfe, 0x1aba86, 0x1b174d,
    0x1b1c53, 0x1b39b, 0x1b65ad, 0x1b81c7, 0x1bbf3c, 0x1c05f6, 0x1c16,
    0x1c2f3b, 0x1c64, 0x1c7882, 0x1ca279, 0x1d29fc, 0x1d4d47, 0x1d4fd7,
    0x1d690e, 0x1d8484, 0x1da78a, 0x1db9a3, 0x1dec9e, 0x1e22e0, 0x1e460c,
    0x1e95ad, 0x1ee5b, 0x1f48e4, 0x1f4a8, 0x1f5ecc, 0x1f6c45, 0x1f88e0,
    0x1faf59, 0x1fc25f, 0x1ffda4, 0x2003fa, 0x200fff, 0x20296c, 0x204631,
    0x20afbc, 0x20f169, 0x20fdc3, 0x210307, 0x211524, 0x21281c, 0x214bb1,
    0x215c6b, 0x218756, 0x218f74, 0x21c14a, 0x21c151, 0x21cbbf, 0x224793,
    0x225d0b, 0x228754, 0x22b59e, 0x22c4ec, 0x22e297, 0x22fce1, 0x22fde2,
    0x23134c, 0x2350a1, 0x236fc, 0x23f029, 0x24409a, 0x24884f, 0x24db7,
    0x24f0b1, 0x268c70, 0x2716d1, 0x27248c, 0x272f86, 0x273760, 0x27c879,
    0x27ce88, 0x27d90d, 0x27edc1, 0x2817d3, 0x281dc4, 0x283b3b, 0x285b3d,
    0x2a554a, 0x2aba20, 0x2ad77b, 0x2afe70, 0x2b2a62, 0x2be048, 0x2c8651,
    0x2cd3f3, 0x2cd9b3, 0x2cf6bf, 0x2dbb72, 0x2de72b, 0x2df73b, 0x2e3cc0,
    0x2f111a, 0x2f383b, 0x2f7343, 0x2f89f3, 0x2f9db3, 0x2fb3a7, 0x301c59,
    0x306cad, 0x3143a4, 0x31adab, 0x31ba4b, 0x31bac2, 0x320d06, 0x3327ce,
    0x33729, 0x338df8, 0x33b00d, 0x33c24e, 0x33c744, 0x33d1b0, 0x346fc1,
    0x34ef2c, 0x35508e, 0x3555ec, 0x359f55, 0x36307, 0x36524a, 0x3677ba,
    0x368467, 0x368d0b, 0x37449c, 0x378855, 0x378a17, 0x37edea, 0x37efef,
    0x38335d, 0x383fd5, 0x3847b3, 0x38525c, 0x3871d7, 0x387d0f, 0x3889dc,
    0x38aeaa, 0x38d123, 0x38ddff, 0x39192e, 0x393dde, 0x3948bc, 0x395b90,
    0x3963e2, 0x3987ee, 0x39ccad, 0x39ceeb, 0x39fcfb, 0x3a1bb8, 0x3a27f8,
    0x3a5582, 0x3a5cc, 0x3a6305, 0x3ab33c, 0x3abfdd, 0x3ac690, 0x3ac703,
    0x3ad8df, 0x3afb1a, 0x3b1196, 0x3b1c47, 0x3b2fed, 0x3b6703, 0x3b671d,
    0x3b9835, 0x3b9f85, 0x3bb0d2, 0x3bc720, 0x3be4e8, 0x3bf5db, 0x3c2756,
    0x3c2a5e, 0x3c2ae9, 0x3c4bd7, 0x3c564c, 0x3c8dca, 0x3cb5f5, 0x3ce86e,
    0x3ce964, 0x3d03e5, 0x3d0621, 0x3d4df0, 0x3d648b, 0x3d73b3, 0x3d7423,
    0x3d7ccf, 0x3dc98, 0x3dcc4f, 0x3df271, 0x3e00e5, 0x3e1752, 0x3e4862,
    0x3e529c, 0x3e7a7d, 0x3e80b2, 0x3e82da, 0x3e84e9, 0x3e8f08, 0x3eb764,
    0x3ed746, 0x3ef180, 0x3f07f9, 0x3f0a76, 0x3f3449, 0x3f6700, 0x3f6d4d,
    0x3f8029, 0x3fbb79, 0x3fc4f0, 0x3fcb91, 0x400005ea, 0x4000196d,
    0x40003214, 0x4000bb12, 0x4000cf8b, 0x4000d595, 0x4000d648, 0x4000d8f1,
    0x400100fa, 0x4001059c, 0x40011404, 0x40013382, 0x40013ad6, 0x40016ae6,
    0x40017491, 0x40017d50, 0x40018c16, 0x4001a1e2, 0x4001ae30, 0x4001cb61,
    0x4001da64, 0x4001df6d, 0x4001f179, 0x40020fea, 0x40021b9e, 0x40021fec,
    0x40023b4a, 0x40024171, 0x4002c2b1, 0x4002fd53, 0x40036f17, 0x4003cb68,
    0x4003dd98, 0x4004150e, 0x40045f43, 0x40047935, 0x4004a9ab, 0x4004d9d9,
    0x4004eec4, 0x40051f98, 0x4006c063, 0x4006ed7e, 0x40071d05, 0x400741bc,
    0x4007e348, 0x40082df4, 0x4008424c, 0x400885a6, 0x40097a8e, 0x400996ce,
    0x4009a4cf, 0x4009e971, 0x400a3079, 0x400a4f1b, 0x400ada69, 0x400b31fc,
    0x400b4811, 0x400b5166, 0x400b845b, 0x400beef9, 0x400c4e0a, 0x400c529a,
    0x400c5fa5, 0x400c952e, 0x400c9c89, 0x400ca0df, 0x400d304e, 0x400d4438,
    0x400d70ca, 0x400d878c, 0x400da7fe, 0x400e5cfe, 0x400e8a65, 0x400e8bde,
    0x400ed108, 0x400efaba, 0x400f145c, 0x400f7ccd, 0x400fa294, 0x4010408c,
    0x401056b6, 0x4010a1e9, 0x4010ccf0, 0x4010fa3a, 0x401283a7, 0x40138494,
    0x4013de04, 0x401404d0, 0x401439bd, 0x4014709f, 0x40152cab, 0x40157690,
    0x40158f6e, 0x40166d00, 0x4016d311, 0x40171bdd, 0x40172a72, 0x40174283,
    0x4017a2b6, 0x4017ab78, 0x401874ae, 0x40189b99, 0x4018b1c7, 0x4018d374,
    0x4019b003, 0x4019d323, 0x401a12be, 0x401aa0bd, 0x401af2ba, 0x401af3e2,
    0x401af413, 0x401b0ddb, 0x401b25ca, 0x401b329c, 0x401bc411, 0x401bcb89,
    0x401bcd0c, 0x401bf169, 0x401c056b, 0x401caa59, 0x401cb970, 0x401cee78,
    0x401d20a1, 0x401d25a3, 0x401d484b, 0x401d6627, 0x401d8446, 0x401d88a8,
    0x401d9fba, 0x401da25f, 0x401dfd09, 0x401e0d63, 0x401e278d, 0x401e3e86,
    0x401e64d4, 0x401ed1ea, 0x401ed9f5, 0x401f35e4, 0x40201ddc, 0x40207909,
    0x40209177, 0x4020b87a, 0x4020c813, 0x40214a07, 0x40215cce, 0x40215d9c,
    0x4021eca5, 0x4022105b, 0x4023820a, 0x402535d6, 0x40255a5a, 0x402578f4,
    0x40259d62, 0x4025bae0, 0x4025cb2d, 0x4025d602, 0x402686ab, 0x40272546,
    0x40275a7f, 0x4027e83c, 0x4027ea46, 0x402844d7, 0x4028f426, 0x40292935,
    0x40297fee, 0x402980ba, 0x40298e5f, 0x4029918f, 0x4029aa3c, 0x4029b163,
    0x4029bf8c, 0x4029cf61, 0x402a87f2, 0x402a9d3f, 0x402aa5b3, 0x402abc1a,
    0x402abdad, 0x402b4491, 0x402b4b8c, 0x402b4ed8, 0x402b5dba, 0x402b658a,
    0x402bf82f, 0x402c148b, 0x402c340f, 0x402d5b4e, 0x402ef09f, 0x402f7b5c,
    0x402f8a87, 0x4030a921, 0x4031624a, 0x40317e1e, 0x4031d42e, 0x4032292a,
    0x40326d94, 0x4032ed8d, 0x40330c73, 0x40331ed8, 0x40334d79, 0x403386da,
    0x4033a57a, 0x4033de44, 0x4033ef66, 0x4033fb1c, 0x40341263, 0x40342d23,
    0x4034dbfe, 0x403518e3, 0x40353525, 0x403583d4, 0x4035cb01, 0x4035e4e9,
    0x4035eddb, 0x4035f70f, 0x4035ff1b, 0x40366406, 0x4036792e, 0x4036a3ba,
    0x4036ace4, 0x40371af2, 0x40374d63, 0x40377e91, 0x4037ddfd, 0x4037f59f,
    0x4037fdf7, 0x40381ec4, 0x40382c95, 0x40387598, 0x40387e8a, 0x4038f535,
    0x403930cc, 0x40393168, 0x40394048, 0x40394281, 0x40395efc, 0x40395fb6,
    0x40398355, 0x40398b1c, 0x4039a890, 0x403a0d97, 0x403a8849, 0x403ab46e,
    0x403ab4ae, 0x403ac403, 0x403ad4a5, 0x403b0df4, 0x403b1590, 0x403b1b1c,
    0x403b1be1, 0x403b2104, 0x403b2629, 0x403b424d, 0x403bc027, 0x403bde05,
    0x403c1204, 0x403c16f6, 0x403c244f, 0x403c3166, 0x403c3ea3, 0x403c3fae,
    0x403ccdf6, 0x403cd0ef, 0x403cd396, 0x403d9336, 0x403da1f0, 0x403da501,
    0x403daef5, 0x403dbb39, 0x403dcc06, 0x403e58ad, 0x403e73d7, 0x403e7e10,
    0x403e80be, 0x403eff24, 0x403f3e47, 0x48122, 0x4d95, 0x5e026, 0x6bff7,
    0x6c5cc, 0x6d781, 0x75e35, 0x7f47a, 0x80002186, 0x80002273, 0x80002792,
    0x80004d40, 0x8000bc5e, 0x8000c2e4, 0x8000c39d, 0x8001031a, 0x800163c4,
    0x800194ea, 0x8001e2b6, 0x8001fd7f, 0x80025907, 0x80025cbf, 0x80029ece,
    0x8002aff7, 0x8002cbfd, 0x80032ebf, 0x80037993, 0x80039854, 0x8003a880,
    0x8003ab02, 0x800540ef, 0x80057935, 0x800583dd, 0x800590fc, 0x800741b6,
    0x8007744c, 0x80077eaa, 0x8007e022, 0x8007f4f9, 0x800832b0, 0x80087d2f,
    0x800897eb, 0x8008be42, 0x8008f111, 0x80094a0b, 0x8009a943, 0x800a027a,
    0x800a86ad, 0x800b99f0, 0x800ba5b3, 0x800bbf59, 0x800c5255, 0x800c97a5,
    0x800cb0a0, 0x800cdc57, 0x800ce5fe, 0x800dcda3, 0x800df3da, 0x800df3fc,
    0x800e01a2, 0x800e517d, 0x800e64d5, 0x800e89aa, 0x800e8b51, 0x800fb17e,
    0x800fe298, 0x80100b99, 0x80101ea8, 0x801020cf, 0x8010525c, 0x80108759,
    0x8010bc00, 0x801183ef, 0x8011dd3e, 0x80120a09, 0x80133eb9, 0x801423cb,
    0x801445f4, 0x80146c4f, 0x8015a72c, 0x8015f185, 0x8016606b, 0x80166d3d,
    0x80168256, 0x801745d4, 0x8017e32d, 0x80184584, 0x8018a6cf, 0x80195e57,
    0x80196e4e, 0x8019a722, 0x8019b1d5, 0x8019e3b9, 0x801ac398, 0x801ae786,
    0x801af45c, 0x801b0103, 0x801b0111, 0x801b6dc2, 0x801b6de2, 0x801b7600,
    0x801b84c6, 0x801bb6e5, 0x801bbe73, 0x801bfc57, 0x801c7c92, 0x801c9181,
    0x801c9d72, 0x801cdb29, 0x801d023a, 0x801d16cb, 0x801d227c, 0x801d34ac,
    0x801d3fb3, 0x801d58ce, 0x801d6569, 0x801dabb7, 0x801dc853, 0x801e529f,
    0x801edbed, 0x801eef76, 0x801f02d3, 0x801f03e3, 0x801f4a0a, 0x801f7d8e,
    0x801f9f7a, 0x801fc084, 0x802011b4, 0x80201479, 0x8020349e, 0x80208832,
    0x8020c23b, 0x8020c979, 0x8020ca8d, 0x802128b7, 0x802138e3, 0x80215e3d,
    0x802167cc, 0x8021cb48, 0x8021d6c4, 0x8021f9c0, 0x8021fb23, 0x8022017b,
    0x8022080d, 0x80223911, 0x80223ad2, 0x80226264, 0x8022729d, 0x80227845,
    0x802441b0, 0x8024905b, 0x8024c635, 0x8024e7e8, 0x802526d0, 0x80254eb0,
    0x8025547e, 0x80257d83, 0x8025af98, 0x8025b25e, 0x8025c899, 0x8025cb81,
    0x8025ebf5, 0x8025edf4, 0x80263499, 0x802668cd, 0x80267174, 0x80268ad1,
    0x8026baf6, 0x8026bea0, 0x802761f2, 0x8027c3d0, 0x8027f95d, 0x80280bfc,
    0x80281a19, 0x8028386d, 0x8028f03d, 0x8029789d, 0x8029c5e0, 0x802ac083,
    0x802acc69, 0x802b03d3, 0x802b1d65, 0x802b51c5, 0x802b9db8, 0x802c2b0b,
    0x802c514c, 0x802d6307, 0x802d8085, 0x802df63a, 0x802f6be4, 0x802fe259,
    0x80302197, 0x803021cc, 0x803079dd, 0x8030c855, 0x80320d11, 0x803211a2,
    0x803220d0, 0x80323a73, 0x80325a85, 0x803283e1, 0x80328dc9, 0x8032b710,
    0x8033a008, 0x8033a152, 0x8033dad5, 0x8033e4b2, 0x8033ed4a, 0x803422e7,
    0x803430e0, 0x8034626e, 0x80346f34, 0x80347152, 0x8034c659, 0x8034eed8,
    0x80353a12, 0x803541aa, 0x80355bb6, 0x8035628d, 0x803575fb, 0x80358cf4,
    0x8035bc35, 0x8035c466, 0x8035eba5, 0x80362609, 0x803694b1, 0x8036a131,
    0x8036e77e, 0x8036f210, 0x80377f7f, 0x8037a994, 0x8037c4d0, 0x8037fb73,
    0x8037fbfb, 0x8038005f, 0x8038332b, 0x8038a13c, 0x8038a216, 0x8038afcb,
    0x8038cae0, 0x80391069, 0x803927b9, 0x80393f3d, 0x80394216, 0x80396c67,
    0x80397a72, 0x80397ad1, 0x8039b68a, 0x8039d7e7, 0x8039dff4, 0x8039e8e4,
    0x803a0d9c, 0x803a2d3b, 0x803a4501, 0x803a4678, 0x803a4748, 0x803a596d,
    0x803a7b8e, 0x803a7ef4, 0x803acb2c, 0x803acbb8, 0x803ae81c, 0x803ae8cf,
    0x803aebf0, 0x803af8ee, 0x803b0d68, 0x803b2069, 0x803b2307, 0x803b242c,
    0x803b2fdb, 0x803b38e4, 0x803b4753, 0x803b479f, 0x803b48e1, 0x803b6d24,
    0x803b9184, 0x803b9378, 0x803b9524, 0x803b9b4f, 0x803b9ddf, 0x803b9e6a,
    0x803ba1db, 0x803ba5a8, 0x803bb201, 0x803bc475, 0x803bcce2, 0x803bebc6,
    0x803c053d, 0x803c111d, 0x803c207c, 0x803c5ebc, 0x803c6e84, 0x803c83b3,
    0x803cba41, 0x803d0225, 0x803d0dbe, 0x803d64fd, 0x803d7bb9, 0x803d8440,
    0x803dcfb4, 0x803dd166, 0x803dfc06, 0x803e064d, 0x803e1fba, 0x803e5d32,
    0x803e6284, 0x803e63d7, 0x803e7ef4, 0x803e8e7d, 0x812cb, 0x8d82, 0x99bab,
    0xa2616, 0xb1b7a, 0xb4ac4, 0xb56c2, 0xb5e56, 0xc0001799, 0xc0005733,
    0xc0006f85, 0xc000b3e8, 0xc000be95, 0xc000ea4b, 0xc0010b02, 0xc0010b92,
    0xc0014f9e, 0xc00171df, 0xc0017947, 0xc001b4d2, 0xc001f8bf, 0xc00217f2,
    0xc0022349, 0xc0022e1a, 0xc0026759, 0xc002be8a, 0xc002f869, 0xc0034ace,
    0xc00352c4, 0xc003ad93, 0xc004401c, 0xc004949f, 0xc004a314, 0xc004df62,
    0xc0050686, 0xc0069806, 0xc006bfba, 0xc006dc9f, 0xc006f1db, 0xc007584f,
    0xc007d49b, 0xc0080d39, 0xc0083750, 0xc008bd53, 0xc008f0cd, 0xc0091992,
    0xc009a50f, 0xc00a0e9c, 0xc00a3929, 0xc00a7351, 0xc00ae923, 0xc00b4176,
    0xc00b4b33, 0xc00b4bdc, 0xc00b7386, 0xc00b8095, 0xc00b8f45, 0xc00b9fd6,
    0xc00be393, 0xc00c3ab1, 0xc00c4234, 0xc00c4600, 0xc00ca18f, 0xc00ce8e9,
    0xc00d1cb7, 0xc00dd45b, 0xc00de715, 0xc00e10fa, 0xc00e155c, 0xc00e2da9,
    0xc00f3eb7, 0xc00fa9e8, 0xc00fb02b, 0xc00fea0c, 0xc00fed2e, 0xc01035b9,
    0xc011392b, 0xc01169b9, 0xc011fbb3, 0xc0126e35, 0xc012e1a5, 0xc013d10a,
    0xc014d605, 0xc01543fd, 0xc01568a6, 0xc015f218, 0xc015f7ed, 0xc015fe5e,
    0xc0161932, 0xc01674b8, 0xc016abd4, 0xc016ca7a, 0xc0170cd2, 0xc0173406,
    0xc017a3b9, 0xc017bad8, 0xc018000c, 0xc0182f0c, 0xc018b228, 0xc0199993,
    0xc01999b7, 0xc019a2ae, 0xc01a4366, 0xc01a84d6, 0xc01ae6c7, 0xc01af45f,
    0xc01af5a7, 0xc01b0e32, 0xc01c0cef, 0xc01c4269, 0xc01c642d, 0xc01c96b7,
    0xc01c9c97, 0xc01c9da1, 0xc01ca6ac, 0xc01d14e8, 0xc01d679f, 0xc01da790,
    0xc01df32e, 0xc01df3cb, 0xc01e1f8c, 0xc01e472d, 0xc01e7b51, 0xc01e7b5c,
    0xc01e87d4, 0xc01f424a, 0xc01f6bef, 0xc01f763c, 0xc01f7bad, 0xc01f8517,
    0xc01fab93, 0xc01fccb7, 0xc020246e, 0xc02041a4, 0xc0206234, 0xc0206edb,
    0xc020771f, 0xc0209cf7, 0xc020ddca, 0xc021e62d, 0xc022a177, 0xc022c278,
    0xc022f3ff, 0xc02357ec, 0xc0237fb3, 0xc02392bf, 0xc023a632, 0xc023b4fa,
    0xc0241c1b, 0xc0243087, 0xc0247b93, 0xc02568cf, 0xc025aeed, 0xc025ba41,
    0xc025da65, 0xc025ff7d, 0xc026052d, 0xc0268268, 0xc0273890, 0xc0276b43,
    0xc0276b99, 0xc02779cb, 0xc027b0e2, 0xc0287fbf, 0xc028a53d, 0xc028c81e,
    0xc028e0df, 0xc02953b9, 0xc029818c, 0xc02a2e86, 0xc02a3753, 0xc02a57ba,
    0xc02a744e, 0xc02a756e, 0xc02a8883, 0xc02a91f9, 0xc02afe43, 0xc02b032a,
    0xc02b3ac4, 0xc02b7b92, 0xc02bbeda, 0xc02bd6b0, 0xc02c3898, 0xc02c7404,
    0xc02c8719, 0xc02cdf1d, 0xc02ce93c, 0xc02cfbb7, 0xc02d2819, 0xc02ddac7,
    0xc02e83f4, 0xc02e9dfb, 0xc02ecaf0, 0xc02ee103, 0xc02f1c76, 0xc03054e6,
    0xc030abd2, 0xc030bd49, 0xc0316e12, 0xc031c031, 0xc031cf29, 0xc031e87e,
    0xc031f055, 0xc0320324, 0xc032d580, 0xc0331636, 0xc0333845, 0xc0334480,
    0xc033c82f, 0xc033cb91, 0xc034a1aa, 0xc034b685, 0xc034b97f, 0xc034c235,
    0xc034cd52, 0xc0356854, 0xc03588bd, 0xc035895a, 0xc035a126, 0xc035a55e,
    0xc035f6a4, 0xc0360477, 0xc0362073, 0xc03641b9, 0xc0365707, 0xc0368b42,
    0xc0368c0f, 0xc03690ce, 0xc036a97b, 0xc036be5c, 0xc036d7d2, 0xc036dc81,
    0xc036e3bf, 0xc037127f, 0xc0371f06, 0xc037320f, 0xc0373b4c, 0xc0374271,
    0xc0376111, 0xc0377ae9, 0xc037874c, 0xc0378f62, 0xc037c298, 0xc037c86a,
    0xc037cd59, 0xc0380e21, 0xc0381c1c, 0xc0381efc, 0xc03860ac, 0xc0388a30,
    0xc0389bfe, 0xc038ac34, 0xc038ca11, 0xc038cbea, 0xc038da07, 0xc038da43,
    0xc038ef2a, 0xc03933eb, 0xc0395b23, 0xc0397981, 0xc0399e1c, 0xc039a7cf,
    0xc039b786, 0xc039b788, 0xc039bfb1, 0xc039c130, 0xc039c287, 0xc039c28b,
    0xc039d34a, 0xc039e420, 0xc039eb27, 0xc039ef0b, 0xc03a0adf, 0xc03a0f69,
    0xc03a22a4, 0xc03a266e, 0xc03a3fbd, 0xc03a5ceb, 0xc03a5cfb, 0xc03a7f97,
    0xc03a8270, 0xc03a948f, 0xc03ab570, 0xc03aca56, 0xc03ada37, 0xc03b0403,
    0xc03b2f39, 0xc03bc19b, 0xc03bc502, 0xc03c1a66, 0xc03c2746, 0xc03c2c93,
    0xc03c5ddd, 0xc03c6ef0, 0xc03c8b24, 0xc03c9041, 0xc03cf5a8, 0xc03d1c93,
    0xc03d5156, 0xd22e0, 0xd5494, 0xd867b, 0xdaefb, 0xdcd20, 0xddc3, 0xf7705,
    0xf7915, 0xfa055, 0xfab86,
  }
},
{ discohash1::DISCoHAsH<128>,       128, 0x05C0460C, "discohash1-128",   "DISCoHAsH v2", GOOD, {} },
{ discohash2::DISCoHAsH_2<64>,      64, 0x8FF45ABF, "discohash2",   "DISCoHAsH v2", GOOD, {} },
{ discohash2::DISCoHAsH_2<128>,     128, 0x95E58C14, "discohash2-128",   "DISCoHAsH v2", GOOD, {} },
{ DISCoHAsH_512_64,     64, 0x9182A886, "discoNONG",   "discoNONG - discohash with 512-bit internal state 64-bit output", GOOD,
  {
    0x100a32, 0x1048e7, 0x104a1, 0x106bba, 0x10a6c, 0x10c526, 0x10cd90, 0x10d00f,
    0x10e4e, 0x10e714, 0x11736, 0x11769, 0x11eac0, 0x12000a, 0x1200d,
    0x1205b, 0x12149, 0x121766, 0x12241, 0x122bf5, 0x12775, 0x128184,
    0x12852f, 0x12c69a, 0x12c74b, 0x12d5ea, 0x12e2c6, 0x12e7c6, 0x1301c8,
    0x132478, 0x133710, 0x133a36, 0x138f97, 0x13c4b7, 0x13d621, 0x13fb54,
    0x14149f, 0x141cd5, 0x1436f5, 0x1455a, 0x146fe9, 0x148428, 0x14b397,
    0x14d46b, 0x14e77, 0x14f6c2, 0x150700, 0x1519c1, 0x156442, 0x158339,
    0x15a812, 0x15b4ff, 0x15d6b2, 0x15d712, 0x15d83f, 0x15df1e, 0x15e058,
    0x161563, 0x161902, 0x161e97, 0x16325f, 0x166219, 0x16a5fa, 0x16ddf6,
    0x171163, 0x172e15, 0x174283, 0x175563, 0x175d72, 0x177697, 0x1782e1,
    0x17b06b, 0x182c1c, 0x184346, 0x18439b, 0x184a92, 0x187da6, 0x18b219,
    0x18d4af, 0x190c0c, 0x190c26, 0x197878, 0x19c49c, 0x1a1bbd, 0x1a20f0,
    0x1a2499, 0x1a2583, 0x1a5ecb, 0x1a704, 0x1adcc9, 0x1b3602, 0x1b47b5,
    0x1b5317, 0x1b9906, 0x1bb3ec, 0x1bfcf8, 0x1c2844, 0x1c3594, 0x1c3c4e,
    0x1c9f3, 0x1ca49, 0x1cc0d3, 0x1ce13e, 0x1cfee9, 0x1d0bcd, 0x1d10e6,
    0x1d31b4, 0x1d3509, 0x1d6904, 0x1e40b5, 0x1e5d7, 0x1e8180, 0x1e8abd,
    0x1f0f46, 0x1f3155, 0x1f3994, 0x1f8d44, 0x1fc9e9, 0x1fe862, 0x1feffc,
    0x20036f, 0x20119, 0x202c25, 0x205cd2, 0x20b914, 0x20c8c0, 0x20ddc2,
    0x2100b2, 0x2170f3, 0x218163, 0x2184cb, 0x2187f5, 0x219940, 0x21fabc,
    0x226cfc, 0x227954, 0x2288df, 0x229d95, 0x22e202, 0x22ef94, 0x22f23d,
    0x233fab, 0x23581d, 0x237e45, 0x238b74, 0x23a0fc, 0x23a53f, 0x23baa4,
    0x23ed32, 0x2434d2, 0x24410c, 0x245589, 0x2507ed, 0x251892, 0x256626,
    0x2571f3, 0x25c0ee, 0x25dad6, 0x261981, 0x2628cb, 0x263d9f, 0x26473f,
    0x26cf, 0x274674, 0x274a41, 0x2757f5, 0x27717e, 0x27901e, 0x27bd84,
    0x27edbd, 0x28144e, 0x282bf9, 0x2870e1, 0x28849a, 0x2898a8, 0x28a399,
    0x28d9db, 0x28d9e6, 0x28dc7d, 0x28dda3, 0x29d91, 0x2f45, 0x3664e,
    0x38712, 0x39354, 0x3a3a5, 0x3b0c8, 0x3b8a8, 0x3e80d, 0x3ec0b, 0x3f8b2,
    0x400004d6, 0x400033f6, 0x4000881d, 0x4000d9a8, 0x40014a1e, 0x4001b017,
    0x4001b206, 0x4001c007, 0x4001c114, 0x4001d1e7, 0x40020029, 0x40020d38,
    0x40021f15, 0x40026ade, 0x40026b2b, 0x40029bb6, 0x4002ae1c, 0x4002e115,
    0x4002f6d0, 0x4002fa33, 0x40032000, 0x40035983, 0x40038b86, 0x4003b1a2,
    0x4003e1cf, 0x40041638, 0x40041dfc, 0x40045884, 0x40047046, 0x40048646,
    0x40049f0d, 0x4004d953, 0x4004f293, 0x4004fe50, 0x400504a2, 0x40050ab3,
    0x4005379b, 0x40058c57, 0x40059169, 0x4005be5f, 0x40063cf6, 0x40064342,
    0x40065261, 0x4006802f, 0x4006921d, 0x4006bb13, 0x4007a366, 0x4008180d,
    0x40081b02, 0x40086951, 0x4008791c, 0x40088803, 0x4008a7c6, 0x4008a83a,
    0x4008abbd, 0x4008b35e, 0x400910f8, 0x40091aaa, 0x40091b1b, 0x40093da0,
    0x40095681, 0x40097ead, 0x4009b460, 0x400a0e27, 0x400a713b, 0x400a741f,
    0x400a8eaf, 0x400ac057, 0x400ac400, 0x400b18eb, 0x400b2f1d, 0x400b360a,
    0x400b3d0d, 0x400b48a9, 0x400b565d, 0x400bded2, 0x400c30fa, 0x400c7042,
    0x400c70ef, 0x400c86e0, 0x400c8d08, 0x400cacd7, 0x400cc605, 0x400cda80,
    0x400d0300, 0x400d1a53, 0x400d23b4, 0x400daaf8, 0x400db7b7, 0x400dfb5b,
    0x400e426c, 0x400e43d0, 0x400ed3f2, 0x400edf06, 0x400f0cb4, 0x400f4706,
    0x400f5449, 0x400f6698, 0x40100dc9, 0x40103c54, 0x401094a4, 0x4010b052,
    0x4010bc1c, 0x4010d5fd, 0x4010ea41, 0x4010fe5c, 0x40110dd9, 0x40113389,
    0x40114daa, 0x4011b20b, 0x4011b348, 0x4011e814, 0x4011f23c, 0x4012234c,
    0x40122953, 0x40126fe9, 0x40127bd3, 0x4012bbed, 0x4012d6d7, 0x4012eeb5,
    0x4012f004, 0x40130644, 0x401310be, 0x40131ecb, 0x401323ae, 0x40132985,
    0x40136463, 0x4014261d, 0x40143a2a, 0x40145451, 0x4014a39e, 0x4014d725,
    0x4014df4f, 0x4015055e, 0x40151a64, 0x401532e2, 0x40155c05, 0x401566d9,
    0x401580b8, 0x401581cc, 0x4015965f, 0x4015a86f, 0x4015a9e0, 0x4015abcd,
    0x4015ac32, 0x4015b306, 0x4015e5e2, 0x4015edd6, 0x4015ffcc, 0x401600c2,
    0x401631e1, 0x40164c67, 0x40165c1c, 0x4016ca58, 0x4016ed83, 0x40171df6,
    0x40172be9, 0x40173907, 0x4017427d, 0x40177534, 0x40178549, 0x40179138,
    0x4017a8f5, 0x4017d6a1, 0x4017e305, 0x4018444f, 0x40184bf3, 0x40186ae0,
    0x40187bc3, 0x40188cb2, 0x4018bc67, 0x4018d057, 0x40194f2a, 0x40195cda,
    0x40195ee8, 0x40195fbe, 0x401961cc, 0x40196ecc, 0x40198cc8, 0x401997cf,
    0x4019a3d7, 0x4019ba5e, 0x4019c9a2, 0x4019d266, 0x4019e5cf, 0x4019fd52,
    0x401a2a82, 0x401a45dd, 0x401a4958, 0x401a4a49, 0x401a7710, 0x401ab639,
    0x401aba09, 0x401b01d8, 0x401b15b2, 0x401b448d, 0x401b5130, 0x401b731c,
    0x401b84b4, 0x401b8f40, 0x401b9850, 0x401b9e15, 0x401ba330, 0x401bc718,
    0x401bc71b, 0x401be232, 0x401bf946, 0x401c20ec, 0x401c3244, 0x401c554d,
    0x401c7623, 0x401c9b0a, 0x401cc9fd, 0x401d03a3, 0x401d7553, 0x401d7cdb,
    0x401d7d9e, 0x401d8af5, 0x401d91f0, 0x401db4cb, 0x401dd158, 0x401de8d7,
    0x401dfeec, 0x401e089a, 0x401e20a5, 0x401e5452, 0x401e6dc3, 0x401e9700,
    0x401e9876, 0x401e9925, 0x401ea466, 0x401ea81e, 0x401f1235, 0x401f2039,
    0x401f45ca, 0x401f4901, 0x401f6f59, 0x401f71e0, 0x401f931f, 0x401f9812,
    0x401fb0b9, 0x40201647, 0x4020a180, 0x4020c81c, 0x4020d6c3, 0x4020fcfc,
    0x402142ed, 0x4021453a, 0x402162f2, 0x40219532, 0x40219621, 0x402207a9,
    0x40220bbd, 0x402210cd, 0x40222d05, 0x4022551a, 0x402256b9, 0x402296db,
    0x4022e8cc, 0x4022fffe, 0x40231460, 0x402343ee, 0x40234f10, 0x40236a7b,
    0x4024af94, 0x4024c557, 0x4024ccb1, 0x402508fd, 0x4025235f, 0x402526f9,
    0x40252dd8, 0x40252fe8, 0x40254bba, 0x4025f873, 0x4025fe67, 0x40260617,
    0x402620ef, 0x4026a247, 0x4026a6bd, 0x4026bb3b, 0x402734b0, 0x40274931,
    0x40274d47, 0x402758a3, 0x4027598e, 0x40278e15, 0x4027aabd, 0x4027b4bf,
    0x4027cb34, 0x40284c76, 0x40285372, 0x402866f5, 0x40286771, 0x40286780,
    0x402873e5, 0x4028777c, 0x4080d, 0x409c1, 0x43ecd, 0x4461c, 0x44abd,
    0x4a3ad, 0x4c806, 0x53dec, 0x58a56, 0x58adb, 0x58b0d, 0x5970d, 0x59dd7,
    0x59f2, 0x5b9f, 0x5bd4d, 0x61a0, 0x637d, 0x6765a, 0x6bf6, 0x6cf0,
    0x6d217, 0x6db4, 0x6db7, 0x6fa60, 0x70bbc, 0x71ace, 0x7239a, 0x72e69,
    0x72f3e, 0x752c, 0x78b34, 0x78fae, 0x7e95, 0x7ea8c, 0x800016e9,
    0x80002703, 0x80003878, 0x800040a5, 0x800044cc, 0x80007d31, 0x80007eed,
    0x80008379, 0x800083f4, 0x8000869f, 0x8000870d, 0x80008bba, 0x80008e74,
    0x80008f23, 0x80009070, 0x80009b85, 0x80009c6a, 0x8000a031, 0x8000a636,
    0x8000aa0b, 0x8000aabf, 0x8000ace8, 0x8000ae51, 0x8000af96, 0x8000b217,
    0x80018cec, 0x80019b4b, 0x8001c33e, 0x8001d601, 0x8001eb3c, 0x8001f9bd,
    0x800250a5, 0x80028a84, 0x8003087e, 0x800327bd, 0x800332c0, 0x800354cf,
    0x80035535, 0x8003970e, 0x8003aa27, 0x8003b59a, 0x8003be77, 0x8003e2f0,
    0x8003ffa0, 0x8003ffd8, 0x80040dd8, 0x80045f90, 0x8004abb7, 0x80057461,
    0x800579f0, 0x80057f06, 0x8006980c, 0x8006ab30, 0x8006b3b1, 0x8006f6d3,
    0x800745e4, 0x80074e41, 0x80087205, 0x8008ac93, 0x8008d6e1, 0x80092db0,
    0x80095621, 0x80096148, 0x80097827, 0x80098766, 0x800998ba, 0x8009cad9,
    0x8009cbe9, 0x800a2994, 0x800a2f27, 0x800a3403, 0x800a3ec2, 0x800a6554,
    0x800a7c25, 0x800aae01, 0x800ae4c7, 0x800af316, 0x800b42ac, 0x800b4f6f,
    0x800b5177, 0x800b6e98, 0x800b83fd, 0x800bc12b, 0x800bfdfe, 0x800c1d03,
    0x800c3d87, 0x800c6958, 0x800d0879, 0x800d4c59, 0x800d6be5, 0x800dee53,
    0x800df626, 0x800dff56, 0x800e0754, 0x800e1596, 0x800e380d, 0x800e38a5,
    0x800e5774, 0x800ec72f, 0x800ee8fb, 0x800f0b43, 0x800f2910, 0x800f441b,
    0x800f9b54, 0x800fb149, 0x800feb50, 0x800ff03f, 0x800fff06, 0x80101da5,
    0x8010440d, 0x801065ee, 0x80107e2a, 0x801086ee, 0x8010a41d, 0x8010c61e,
    0x8010c9eb, 0x8010effe, 0x8011527e, 0x80117c0b, 0x80119177, 0x80119ec8,
    0x8011a0d4, 0x8011b5df, 0x8011cda8, 0x8011d503, 0x8011d794, 0x8011d9bb,
    0x80120490, 0x80124836, 0x801273ab, 0x8012c426, 0x8012e8b1, 0x8012f077,
    0x8012f8f9, 0x8013437c, 0x80138d62, 0x8013a6c2, 0x8013dcd3, 0x8013e695,
    0x8014122e, 0x80142b1d, 0x8014a83e, 0x8014c2cc, 0x8014fe8f, 0x8015072d,
    0x8015300f, 0x80154d17, 0x80158711, 0x80161ea3, 0x80163486, 0x80164e4a,
    0x80167885, 0x8016971b, 0x80170898, 0x80171ab0, 0x80173b1c, 0x80174e07,
    0x80174f46, 0x801772a3, 0x8017965c, 0x80179b0d, 0x8017bedf, 0x8017e881,
    0x8018349d, 0x80186f29, 0x801883f3, 0x8018b4ff, 0x8018f78d, 0x80196186,
    0x801964e6, 0x80199c70, 0x801a3859, 0x801a4dbd, 0x801a56b1, 0x801a7382,
    0x801ab216, 0x801af663, 0x801af946, 0x801b02e6, 0x801bdff4, 0x801c0dc0,
    0x801c447f, 0x801c460d, 0x801c5e55, 0x801c674d, 0x801c89f5, 0x801c9cda,
    0x801d46fb, 0x801d83fb, 0x801d8563, 0x801d9b53, 0x801da9de, 0x801dedd4,
    0x801e1e96, 0x801e207a, 0x801e5507, 0x801e679d, 0x801e6c61, 0x801e78fd,
    0x801e7ea9, 0x801e9298, 0x801e9343, 0x801e9484, 0x801e9f0d, 0x801ec5ea,
    0x801efad5, 0x801f2f74, 0x801f47d4, 0x801f56a5, 0x801f8e1e, 0x801f96c6,
    0x801f9d43, 0x801febdd, 0x80201802, 0x802028dc, 0x80203a27, 0x80206041,
    0x80207514, 0x80208264, 0x80209187, 0x802095cb, 0x8020c5c7, 0x8020df99,
    0x8020f775, 0x80212100, 0x802135c2, 0x80214341, 0x80214da7, 0x80217f55,
    0x80217f93, 0x80219f52, 0x80219fa0, 0x8021b56d, 0x8021b66c, 0x8021c203,
    0x8021cebb, 0x8021e2b8, 0x8021e7b3, 0x80222aab, 0x802237f3, 0x80223d22,
    0x80225557, 0x80225e86, 0x80227e28, 0x80228466, 0x8022917c, 0x80229940,
    0x80229d55, 0x8022c4cd, 0x8022d032, 0x8022dc1a, 0x8022e8a5, 0x8022edef,
    0x802326fb, 0x80232f7d, 0x80234d15, 0x80244607, 0x80245286, 0x80248812,
    0x8024e0db, 0x8024f4f5, 0x80251ac9, 0x80251e84, 0x80251f3c, 0x80254e1c,
    0x80257048, 0x8025d064, 0x802605f8, 0x802635e2, 0x80264d90, 0x802723d5,
    0x80273429, 0x80274286, 0x80274ac6, 0x802756b5, 0x80275f1e, 0x80277f48,
    0x80280cd4, 0x80280d54, 0x80282810, 0x802832ea, 0x80285cf0, 0x802886ec,
    0x8028bdde, 0x8028c22c, 0x8028cd15, 0x844b5, 0x8587, 0x8979, 0x8a1d7,
    0x8a565, 0x8b6e2, 0x91e37, 0x923bd, 0x92925, 0x92dd9, 0x9352d, 0x94477,
    0x944b1, 0x94578, 0x980dd, 0x98e9, 0x9a45, 0x9a6f6, 0x9d262, 0x9decf,
    0x9f28, 0xa0268, 0xa103, 0xa2477, 0xa2d18, 0xa3934, 0xa5488, 0xa5d52,
    0xa61cd, 0xa87eb, 0xa8cdd, 0xa9507, 0xa9bef, 0xac840, 0xad013, 0xae24,
    0xaef5, 0xb360b, 0xb3bb4, 0xb467b, 0xbe91f, 0xc000072d, 0xc00057e7,
    0xc0008098, 0xc0009247, 0xc0009ae3, 0xc000af4c, 0xc000c097, 0xc000c6ba,
    0xc00115b7, 0xc0011bea, 0xc00125f5, 0xc0012907, 0xc001313a, 0xc00136ba,
    0xc001371b, 0xc001376d, 0xc0013bd5, 0xc0013e47, 0xc0013ff0, 0xc0014610,
    0xc0014924, 0xc0014a17, 0xc0014a47, 0xc0014c4d, 0xc0014cea, 0xc0015462,
    0xc00157a5, 0xc001586a, 0xc0015e58, 0xc00160b7, 0xc001651c, 0xc0016767,
    0xc001679e, 0xc00167e5, 0xc0016a99, 0xc001757b, 0xc0018760, 0xc00187b1,
    0xc0018858, 0xc001978a, 0xc00198dc, 0xc001a043, 0xc001c927, 0xc001dbc9,
    0xc0021297, 0xc0021d12, 0xc0027844, 0xc002cb7c, 0xc003125e, 0xc00357a9,
    0xc003849f, 0xc0038584, 0xc003a0e8, 0xc003a8d8, 0xc004350f, 0xc00466f1,
    0xc00472c6, 0xc004a741, 0xc004cb40, 0xc004cef8, 0xc004d82d, 0xc005377c,
    0xc0053ec2, 0xc0054f90, 0xc0057894, 0xc0059baf, 0xc005f045, 0xc0061c62,
    0xc0066aed, 0xc006a379, 0xc006a921, 0xc007086f, 0xc007101a, 0xc0071043,
    0xc00735b9, 0xc0073be3, 0xc0079581, 0xc007adff, 0xc007ce95, 0xc007e2bf,
    0xc007f15b, 0xc0084c35, 0xc0086865, 0xc008748a, 0xc008777d, 0xc008db4b,
    0xc008dfef, 0xc009080a, 0xc009427b, 0xc009940f, 0xc009a2ee, 0xc009bb0b,
    0xc009d27f, 0xc009e521, 0xc009f8cf, 0xc00a14bb, 0xc00a1b43, 0xc00a1ea3,
    0xc00a79fe, 0xc00a8ed0, 0xc00ad619, 0xc00af3a5, 0xc00b51de, 0xc00b60f3,
    0xc00bfd32, 0xc00c0c82, 0xc00c3267, 0xc00c4a2a, 0xc00c9912, 0xc00c9b24,
    0xc00cc69f, 0xc00cd6be, 0xc00ce4cb, 0xc00cfceb, 0xc00d1f5b, 0xc00dd183,
    0xc00df851, 0xc00e08e0, 0xc00e2012, 0xc00e28be, 0xc00e4f02, 0xc00e5618,
    0xc00e8a87, 0xc00eb451, 0xc00f110c, 0xc00f1ccf, 0xc00f1fbf, 0xc00f3b02,
    0xc00f9ffb, 0xc00fa7fc, 0xc0103527, 0xc0104c31, 0xc0105aa3, 0xc01066e8,
    0xc010aed2, 0xc010b62c, 0xc010c0e0, 0xc010f6fc, 0xc010f75c, 0xc0111b50,
    0xc011799c, 0xc011947f, 0xc0119f34, 0xc011a43d, 0xc011ab1c, 0xc011e624,
    0xc0121996, 0xc01223ea, 0xc01234b7, 0xc0124ed6, 0xc0127c7f, 0xc012cd4c,
    0xc0133dde, 0xc0133e30, 0xc013ccfc, 0xc013e50a, 0xc01420d8, 0xc014654f,
    0xc014da0b, 0xc014e924, 0xc014f718, 0xc014fdb6, 0xc01520c1, 0xc0152612,
    0xc015507d, 0xc0156f86, 0xc015959b, 0xc015b466, 0xc015cfdf, 0xc015dfb2,
    0xc0160f02, 0xc0168414, 0xc016ae71, 0xc016afc7, 0xc016c062, 0xc016c302,
    0xc0170c5e, 0xc0174100, 0xc017902b, 0xc01795e3, 0xc017b921, 0xc017fcc3,
    0xc018f22a, 0xc0190bf3, 0xc0194e86, 0xc0195cfa, 0xc01965ba, 0xc0196665,
    0xc019683a, 0xc0196fb9, 0xc0199245, 0xc0199838, 0xc019bc46, 0xc019f898,
    0xc01a0101, 0xc01a679f, 0xc01a812a, 0xc01ab4ed, 0xc01ac16a, 0xc01ae121,
    0xc01b96c2, 0xc01b9beb, 0xc01bf42f, 0xc01c0406, 0xc01c3356, 0xc01c79d4,
    0xc01ccc3d, 0xc01ce1be, 0xc01d0df7, 0xc01d6d0f, 0xc01d7689, 0xc01d9eee,
    0xc01da04c, 0xc01da1dd, 0xc01e0249, 0xc01e1203, 0xc01e4019, 0xc01e843a,
    0xc01ea9b3, 0xc01f0268, 0xc01f0544, 0xc01f3cff, 0xc01f5365, 0xc01f5922,
    0xc01f7fb5, 0xc01f8087, 0xc01fa082, 0xc0201207, 0xc0202b5a, 0xc0204f51,
    0xc0205ab7, 0xc0205f93, 0xc020df2b, 0xc021095f, 0xc0210eff, 0xc0211b2c,
    0xc02128d7, 0xc02158c5, 0xc02167df, 0xc0217784, 0xc021c2b4, 0xc021d2c9,
    0xc021db2f, 0xc021dc61, 0xc021dfbc, 0xc021ecd1, 0xc021fc20, 0xc02228fb,
    0xc02248cc, 0xc0224c97, 0xc02254a2, 0xc0227600, 0xc0229922, 0xc0229b79,
    0xc0229f19, 0xc022a82b, 0xc0231f26, 0xc02390ee, 0xc023bf0b, 0xc023d666,
    0xc024070c, 0xc024151e, 0xc02421bc, 0xc0246379, 0xc0246f96, 0xc0247152,
    0xc024882a, 0xc0249587, 0xc024a721, 0xc024c01c, 0xc024c3e9, 0xc024daa9,
    0xc024ea82, 0xc024f437, 0xc024f992, 0xc0251146, 0xc02511f4, 0xc0252000,
    0xc0253602, 0xc026131e, 0xc02656b0, 0xc02668ef, 0xc0267214, 0xc026996a,
    0xc026a83c, 0xc026c4e8, 0xc026ee57, 0xc0270111, 0xc0270790, 0xc0270914,
    0xc02738a6, 0xc02748e1, 0xc0275969, 0xc0276f3c, 0xc02772da, 0xc0277cfe,
    0xc027e7d8, 0xc027e91f, 0xc027f01b, 0xc027f193, 0xc02802f2, 0xc3b9,
    0xc49e5, 0xc4d83, 0xc62e7, 0xc6a77, 0xc6d30, 0xc6dcf, 0xc782d, 0xc7f31,
    0xc806d, 0xc8f1d, 0xc9217, 0xcb6e3, 0xccbcd, 0xcf315, 0xd184c, 0xd81a7,
    0xd9549, 0xdc47f, 0xe36ae, 0xe410b, 0xe5041, 0xe63dc, 0xe70c4, 0xe80d1,
    0xeaeb8, 0xeb039, 0xeb560, 0xed043, 0xf0436, 0xff756
 } },
{ fasthash32_test,      32, 0xE9481AFC, "fasthash32",  "fast-hash 32bit", GOOD, {0x880355f21e6d1965ULL} },
{ fasthash64_test,      64, 0xA16231A7, "fasthash64",  "fast-hash 64bit", GOOD, {0x880355f21e6d1965ULL} },
// different verif on gcc vs clang
{ floppsyhash_64,       64, 0x0,        "floppsyhash", "slow hash designed for floating point hardware", GOOD, {} },
{ chaskey_test,         64, 0x81A90131, "chaskey",     "mouha.be/chaskey/ with added seed support", GOOD, {} },
{ siphash_test,         64, 0xC58D7F9C, "SipHash",     "SipHash 2-4 - SSSE3 optimized", GOOD, {} },
{ halfsiphash_test,     32, 0xA7A05F72, "HalfSipHash", "HalfSipHash 2-4, 32bit", GOOD, {} },
{ GoodOAAT_test,        32, 0x7B14EEE5, "GoodOAAT",    "Small non-multiplicative OAAT", GOOD, {0x3b00} },
#ifdef HAVE_INT64
{ prvhash64_64mtest,    64, 0xD37C7E74, "prvhash64_64m", "prvhash64m 4.3 64bit", GOOD, {} },
{ prvhash64_64test,     64, 0xD37C7E74, "prvhash64_64",  "prvhash64 4.3 64bit", GOOD, {} },
{ prvhash64_128test,   128, 0xB447480F, "prvhash64_128", "prvhash64 4.3 128bit", GOOD, {} },
{ prvhash64s_64test,    64, 0x891521D6, "prvhash64s_64", "prvhash64s 4.3 64bit", GOOD, {} }, // seed changes
{ prvhash64s_128test,  128, 0x0199728A, "prvhash64s_128","prvhash64s 4.3 128bit", GOOD, {} }, // seed compiler-specific
#endif
// as in rust and swift:
{ siphash13_test,       64, 0x29C010BF, "SipHash13",   "SipHash 1-3 - SSSE3 optimized", GOOD, {} },
#ifndef _MSC_VER
{ tsip_test,            64, 0x8E48155B, "TSip",        "Damian Gryski's Tiny SipHash variant", GOOD, {} },
#ifdef HAVE_INT64
{ seahash_test,         64, 0xF0374078, "seahash",     "seahash (64-bit, little-endian)", GOOD, {} },
{ seahash32low,         32, 0x712F0EE8, "seahash32low","seahash - lower 32bit", GOOD, {} },
#endif /* HAVE_INT64 */
#endif /* !MSVC */
#if defined(HAVE_SSE42) && defined(__x86_64__)
{ clhash_test,          64, 0x0, "clhash",      "carry-less mult. hash -DBITMIX (64-bit for x64, SSE4.2)", GOOD,
  {0xb3816f6a2c68e530, 711} },
#endif
#ifdef HAVE_HIGHWAYHASH
{ HighwayHash64_test,   64, 0x0,        "HighwayHash64", "Google HighwayHash (portable with dylib overhead)", GOOD, {} },
#endif
#if __WORDSIZE >= 64
{ MurmurHash3_x64_128, 128, 0x6384BA69, "Murmur3F",    "MurmurHash3 for x64, 128-bit", GOOD, {0x87c37b91114253d5ULL} },
#endif
#if defined __aarch64__
#define MUM_VERIF            0x280B2CC6
#define MUMLOW_VERIF         0xB13E0239
#elif defined(__GNUC__) && UINT_MAX != ULONG_MAX
#define MUM_VERIF            0x3EEAE2D4
#define MUMLOW_VERIF         0x520263F5
#else
#define MUM_VERIF            0xA973C6C0
#define MUMLOW_VERIF         0x7F898826
#endif
{ mum_hash_test,        64, MUM_VERIF,  "MUM",         "github.com/vnmakarov/mum-hash", POOR,
  {0x0} /* !! and many more. too many */ },
{ mum_low_test,         32, MUMLOW_VERIF,"MUMlow",     "github.com/vnmakarov/mum-hash", GOOD,
  {0x11fb062a, 0x3ca9411b, 0x3edd9a7d, 0x41f18860, 0x691457ba} /* !! */ },
#if defined(__GNUC__) && UINT_MAX != ULONG_MAX
#define MIR_VERIF            0x00A393C8
#define MIRLOW_VERIF         0xE320CE68
#else
#define MIR_VERIF            0x422A66FC
#define MIRLOW_VERIF         0xD50D1F09
#endif
#ifdef HAVE_INT64
// improved MUM
{ mirhash_test,         64, MIR_VERIF,    "mirhash",            "mirhash", GOOD,
  {0x0, 0x5e74c778, 0xa521f17b, 0xe0ab70e3} /* 2^36 !! (plus all hi ranges) */ },
{ mirhash32low,         32, MIRLOW_VERIF, "mirhash32low",       "mirhash - lower 32bit", POOR,
  {0x0, 0x5e74c778, 0xa521f17b, 0xe0ab70e3 } /* !! */ },
{ mirhashstrict_test,   64, 0x422A66FC,   "mirhashstrict",      "mirhashstrict (portable, 64-bit, little-endian)", GOOD,
  {0x7fcc747f} /* tested ok */ },
{ mirhashstrict32low,   32, 0xD50D1F09,   "mirhashstrict32low", "mirhashstrict - lower 32bit", POOR,
  {0x7fcc747f} /* !! */ },
#endif
{ CityHash64_low_test,  32, 0xCC5BC861, "City64low",   "Google CityHash64WithSeed (low 32-bits)", GOOD, {} },
#if defined(HAVE_SSE42) && defined(__x86_64__)
{ CityHash128_test,    128, 0x6531F54E, "City128",     "Google CityHash128WithSeed (old)", GOOD, {} },
{ CityHashCrc128_test, 128, 0xD4389C97, "CityCrc128",  "Google CityHashCrc128WithSeed SSE4.2 (old)", GOOD, {} },
#endif

#if defined(__FreeBSD__) || defined(HAVE_ASAN)
#  define FARM64_VERIF        0x0
#  define FARM128_VERIF       0x0
#else
#  define FARM64_VERIF        0xEBC4A679
#  define FARM128_VERIF       0x305C0D9A
#endif
{ FarmHash32_test,      32, 0/*0x2E226C14*/,   "FarmHash32",  "Google FarmHash32WithSeed", GOOD, {0x2b091701} /* !! */},
{ FarmHash64_test,      64, FARM64_VERIF, "FarmHash64",  "Google FarmHash64WithSeed", GOOD, {} },
//{ FarmHash64noSeed_test,64, 0xA5B9146C,  "Farm64noSeed","Google FarmHash64 without seed (default, misses on final avalanche)", POOR, {} },
{ FarmHash128_test,    128, FARM128_VERIF,"FarmHash128", "Google FarmHash128WithSeed", GOOD, {} },
#if defined(__SSE4_2__) && defined(__x86_64__)
{ farmhash32_c_test,    32, 0/*0xA2E45238*/,   "farmhash32_c", "farmhash32_with_seed (C99)", GOOD,
  {0x2b091701} /* !! */},
{ farmhash64_c_test,    64, FARM64_VERIF, "farmhash64_c",  "farmhash64_with_seed (C99)", GOOD, {} },
{ farmhash128_c_test,  128, FARM128_VERIF,"farmhash128_c", "farmhash128_with_seed (C99)", GOOD, {} },
#endif

{ xxHash64_test,        64, 0x024B7CF4, "xxHash64",    "xxHash, 64-bit", GOOD, {} },
#if 0
{ xxhash256_test,       64, 0x024B7CF4, "xxhash256",   "xxhash256, 64-bit unportable", GOOD, {} },
#endif
{ SpookyHash32_test,    32, 0x3F798BBB, "Spooky32",    "Bob Jenkins' SpookyHash, 32-bit result", GOOD,
  { 0x111af082, 0x26bb3cda, 0x94c4f96c, 0xec24c166 } /* !! */},
{ SpookyHash64_test,    64, 0xA7F955F1, "Spooky64",    "Bob Jenkins' SpookyHash, 64-bit result", GOOD, {} },
{ SpookyHash128_test,  128, 0x8D263080, "Spooky128",   "Bob Jenkins' SpookyHash, 128-bit result", GOOD, {} },
{ SpookyV2_32_test,     32, 0xA48BE265, "SpookyV2_32",  "Bob Jenkins' SpookyV2, 32-bit result", GOOD, {} },
{ SpookyV2_64_test,     64, 0x972C4BDC, "SpookyV2_64",  "Bob Jenkins' SpookyV2, 64-bit result", GOOD, {} },
{ SpookyV2_128_test,   128, 0x893CFCBE, "SpookyV2_128", "Bob Jenkins' SpookyV2, 128-bit result", GOOD, {} },
{ pengyhash_test,       64, 0x1FC2217B, "pengyhash",   "pengyhash", GOOD, {} },
{ mx3hash64_test,       64, 0x4DB51E5B, "mx3",         "mx3 64bit", GOOD, {0x10} /* !! and all & 0x10 */},
#if defined(HAVE_SSE42) &&  (defined(__x86_64__) ||  defined(__aarch64__)) && !defined(_MSC_VER)
{ umash32,              32, 0x9451AF3B, "umash32",     "umash 32", GOOD, {0x90e37057} /* !! */},
{ umash32_hi,           32, 0x0CC4850F, "umash32_hi",  "umash 32 hi", GOOD, {} },
{ umash,                64, 0x161495C6, "umash64",     "umash 64", GOOD, {} },
{ umash128,            128, 0x36D4EC95, "umash128",    "umash 128", GOOD, {} },
#endif
{ halftime_hash_style64_test,  64, 0x0, "halftime_hash64",    "NH tree hash variant", GOOD,
  {0xc61d672b, 0xcc70c4c1798e4a6f, 0xd3833e804f4c574b, 0xecfc1357d65941ae, 0xbe1927f97b8c43f1, 
   0xf4d4beb14ae042bbULL, 0x9a9b4c4e44dd48d1ULL} }, // not vulnerable
{ halftime_hash_style128_test, 64, 0x0, "halftime_hash128",   "NH tree hash variant", GOOD,
  {0xc61d672b, 0xcc70c4c1798e4a6f, 0xd3833e804f4c574b, 0xecfc1357d65941ae, 0xbe1927f97b8c43f1, 
   0xf4d4beb14ae042bbULL, 0x9a9b4c4e44dd48d1ULL} },
{ halftime_hash_style256_test, 64, 0x0, "halftime_hash256",   "NH tree hash variant", GOOD,
  {0xc61d672b, 0xcc70c4c1798e4a6f, 0xd3833e804f4c574b, 0xecfc1357d65941ae, 0xbe1927f97b8c43f1, 
   0xf4d4beb14ae042bbULL, 0x9a9b4c4e44dd48d1ULL} },
{ halftime_hash_style512_test, 64, 0x0, "halftime_hash512",   "NH tree hash variant", GOOD,
  {0xc61d672b, 0xcc70c4c1798e4a6f, 0xd3833e804f4c574b, 0xecfc1357d65941ae, 0xbe1927f97b8c43f1, 
   0xf4d4beb14ae042bbULL, 0x9a9b4c4e44dd48d1ULL} },

{ t1ha2_atonce_test,           64, 0x8F16C948, "t1ha2_atonce",    "Fast Positive Hash (portable", GOOD, {
  } },
{ t1ha2_stream_test,           64, 0xDED9B580, "t1ha2_stream",    "Fast Positive Hash (portable)", POOR, {} },
{ t1ha2_atonce128_test,       128, 0xB44C43A1, "t1ha2_atonce128", "Fast Positive Hash (portable)", GOOD, {} },
{ t1ha2_stream128_test,       128, 0xE929E756, "t1ha2_stream128", "Fast Positive Hash (portable)", POOR, {} },
#if T1HA0_AESNI_AVAILABLE
#  ifndef _MSC_VER
{ t1ha0_ia32aes_noavx_test,  64, 0xF07C4DA5, "t1ha0_aes_noavx", "Fast Positive Hash (AES-NI)", GOOD, {} },
#  endif
#  if defined(__AVX__)
{ t1ha0_ia32aes_avx1_test,   64, 0xF07C4DA5, "t1ha0_aes_avx1",  "Fast Positive Hash (AES-NI & AVX)", GOOD, {} },
#  endif /* __AVX__ */
#  if defined(__AVX2__)
{ t1ha0_ia32aes_avx2_test,   64, 0x8B38C599, "t1ha0_aes_avx2",  "Fast Positive Hash (AES-NI & AVX2)", GOOD, {} },
#  endif /* __AVX2__ */
#endif /* T1HA0_AESNI_AVAILABLE */
#ifdef HAVE_AHASH_C
// aHash does not adhere to a fixed output
{ ahash64_test,         64, 0x00000000, "ahash64",     "ahash 64bit", GOOD, {} },
#endif
{ xxh3_test,            64, 0x39CD9E4A, "xxh3",        "xxHash v3, 64-bit", GOOD, // no known bad seeds
  {0x47ebda34,             // 32bit bad seed
   /* 0xbe4ba423396cfeb8,  // kSecret
   0x396cfeb8, 0xbe4ba423, // kSecret
   0x6782737bea4239b9,     // bitflip1 ^ input
   0xaf56bc3b0996523a,     // bitflip2 ^ input[last 8]
   */
  }},
{ xxh3low_test,         32, 0xFAE8467B, "xxh3low",     "xxHash v3, 64-bit, low 32-bits part", GOOD,
  {0x47ebda34} /* !! */},
{ xxh128_test,         128, 0xEB61B3A0, "xxh128",      "xxHash v3, 128-bit", GOOD,
  {0x47ebda34}},
{ xxh128low_test,       64, 0x54D1CC70, "xxh128low",   "xxHash v3, 128-bit, low 64-bits part", GOOD,
  {0x47ebda34}},
#ifdef HAVE_BIT32
{ wyhash32_test,        32, 0x09DE8066, "wyhash32",    "wyhash v4 (32-bit native)", GOOD,
  { 0x51a43a0f, 0x522235ae, 0x99ac2b20 } },
#else
{ wyhash32low,          32, 0x006B76C4, "wyhash32low", "wyhash v4.1 lower 32bit", GOOD,
  { 0x138d5f9f, 0x1e4f8661, 0x29362732, 0x49a7ee03, 0x4d29ced1, 0x5ee3628c, 0x833f0eb6,
    0x928fce63, 0x99be0ae5, 0xac470842, 0xcaf21e71, 0xfc1c4878 } },
{ wyhash_test,          64, 0xBD5E840C, "wyhash", "wyhash v4.1 (64-bit)", GOOD, {}},
#endif
{ nmhash32_test,        32, 0x12A30553, "nmhash32",       "nmhash32", GOOD, {}},
{ nmhash32x_test,       32, 0xA8580227, "nmhash32x",      "nmhash32x", GOOD, {}},
#ifdef HAVE_KHASHV
#ifdef __clang__ // also gcc 9.4
#define KHASHV32_VERIF  0xB69DF8EB
#define KHASHV64_VERIF  0xA6B7E55B
#else // new gcc-11
#define KHASHV32_VERIF  0 /* 0x9A8F7952 */
#define KHASHV64_VERIF  0 /* 0X90A2A4F9 */
#endif
{ khashv32_test,        32, KHASHV32_VERIF, "k-hashv32",      "Vectorized K-HashV, 32-bit", GOOD, {}},
{ khashv64_test,        64, KHASHV64_VERIF, "k-hashv64",      "Vectorized K-HashV, 64-bit", GOOD, {}},
#endif
{ komihash_test,        64, 0x8157FF6D, "komihash",    "komihash 5.1", GOOD, {} },
{ polymur_test,         64, 0x4F894810, "polymur",     "github.com/orlp/polymur-hash v1", GOOD, {} },
};

HashInfo * findHash ( const char * name )
{
for(size_t i = 0; i < sizeof(g_hashes) / sizeof(HashInfo); i++)
  {
    if(_stricmp(name,g_hashes[i].name) == 0)
      return &g_hashes[i];
  }

  return NULL;
}

// optional hash state initializers
void Hash_init (HashInfo* info) {
  /*
  if (info->hash == sha2_224_64)
    sha224_init(&ltc_state);
  //else if (info->hash == md5_128 || info->hash == md5_32)
  //  md5_init();
  else if (info->hash == rmd128)
    rmd128_init(&ltc_state);
  else
  */
  if(info->hash == tabulation_32_test)
    tabulation_32_init();
#ifdef __SIZEOF_INT128__
  else if(info->hash == multiply_shift ||
          info->hash == pair_multiply_shift)
    multiply_shift_init();
  else if(info->hash == poly_1_mersenne ||
          info->hash == poly_2_mersenne ||
          info->hash == poly_3_mersenne ||
          info->hash == poly_4_mersenne)
    poly_mersenne_init();
  else if(info->hash == tabulation_test)
    tabulation_init();
#endif
#if defined(HAVE_SSE42) && defined(__x86_64__)
  else if(info->hash == clhash_test)
    clhash_init();
  //else if(info->hash == umash32_test ||
  //        info->hash == umash32hi_test ||
  //        info->hash == umash64_test ||
  //        info->hash == umash128_test)
  //  umash_init();
#endif
  else if (info->hash == VHASH_32 || info->hash == VHASH_64)
    VHASH_init();
#ifdef HAVE_HIGHWAYHASH
  else if(info->hash == HighwayHash64_test)
    HighwayHash_init();
#endif
#ifndef _MSC_VER
  else if(info->hash == tsip_test)
    tsip_init();
#endif
  else if(info->hash == chaskey_test)
    chaskey_init();
  else if (info->hash == halftime_hash_style64_test ||
           info->hash == halftime_hash_style128_test ||
           info->hash == halftime_hash_style256_test ||
           info->hash == halftime_hash_style512_test)
    halftime_hash_init();
}

// optional hash seed initializers.
// esp. for Hashmaps, whenever the seed changes, for expensive seeding.
bool Seed_init (HashInfo* info, size_t seed) {
  //Bad_Seed_init (info->hash, seed);
  return Hash_Seed_init (info->hash, seed);
}

// Needed for hashed with a few bad seeds, to reject this seed and generate a new one.
// (GH #99)
void Bad_Seed_init (pfHash hash, uint32_t &seed) {
  // zero-seed hashes:
  if (!seed && (hash == BadHash || hash == sumhash || hash == fletcher2_test ||
                hash == fletcher4_test || hash == Bernstein_test || hash == sdbm_test ||
                hash == JenkinsOOAT_test || hash == JenkinsOOAT_perl_test ||
                hash == SuperFastHash_test || hash == MurmurOAAT_test ||
                hash == o1hash_test))
    seed++;
  else if (hash == Crap8_test && (seed == 0x83d2e73b || seed == 0x97e1cc59))
    seed++;
  else if (hash == MurmurHash1_test && seed == 0xc6a4a793)
    seed++;
  else if (hash == MurmurHash2_test && seed == 0x10)
    seed++;
  else if (hash == MurmurHash2A_test && seed == 0x2fc301c9)
    seed++;
  else if((hash == MurmurHash3_x86_32 || hash == PMurHash32_test) && seed == 0xfca58b2d)
    seed++;
  else if (hash == MurmurHash3_x86_128 && seed == 0x239b961b)
    seed++;
#ifdef HAVE_BIT32
  else if(hash == wyhash32_test)
    wyhash32_seed_init(seed);
#elif defined HAVE_INT64
  //else if(hash == wyhash_test)
  //  wyhash_seed_init(seed);
  else if(hash == wyhash32low)
    wyhash32low_seed_init(seed);
#endif
#ifdef HAVE_INT64
  else if(hash == mirhash_test)
    mirhash_seed_init(seed);
  else if(hash == mirhash32low)
    mirhash32_seed_init(seed);
  else if(hash == mirhashstrict32low && seed == 0x7fcc747f)
    seed++;
  else if(hash == MurmurHash64B_test)
    MurmurHash64B_seed_init(seed);
  else if(hash == discohash1::DISCoHAsH<64>)
    discohash1::DISCoHAsH_64_seed_init(seed);
  else if(hash == DISCoHAsH_512_64)
    DISCoHAsH_512_64_seed_init(seed);
#endif
#ifdef __SIZEOF_INT128__
  else if(hash == multiply_shift)
    multiply_shift_seed_init(seed);
  else if((hash == poly_2_mersenne && seed == 0x60e8512c) ||
          (hash == poly_3_mersenne && seed == 0x3d25f745))
    seed++;
#endif
#if defined(HAVE_SSE42) && defined(__x86_64__)
  else if (hash == clhash_test && seed == 0x0)
    seed++;
#endif
}

bool Hash_Seed_init (pfHash hash, size_t seed) {
  uint32_t seed32 = seed;
  //if (hash == md5_128 || hash == md5_32)
  //  md5_seed_init(seed);
  //if (hash == VHASH_32 || hash == VHASH_64)
  //  VHASH_seed_init(seed);
  if(hash == tabulation_32_test)
    tabulation_32_seed_init(seed);
#ifdef __SIZEOF_INT128__
  else if(hash == multiply_shift || hash == pair_multiply_shift)
    multiply_shift_seed_init(seed32);
  else if(/*hash == poly_0_mersenne || */
          hash == poly_1_mersenne ||
          hash == poly_2_mersenne ||
          hash == poly_3_mersenne ||
          hash == poly_4_mersenne)
    poly_mersenne_seed_init(seed32);
  else if(hash == tabulation_test)
    tabulation_seed_init(seed);
#endif
#if defined(HAVE_SSE42) && defined(__x86_64__)
  else if (hash == clhash_test)
    clhash_seed_init(seed);
# ifndef _MSC_VER  
  else if (hash == umash32 ||
          hash == umash32_hi ||
          hash == umash ||
          hash == umash128)
    umash_seed_init(seed);
# endif
  else if (hash == halftime_hash_style64_test || hash == halftime_hash_style128_test ||
           hash == halftime_hash_style256_test || hash == halftime_hash_style512_test)
    halftime_hash_seed_init(seed);
  /*
  else if(hash == hashx_test)
    hashx_seed_init(info, seed);
  */
  else if(hash == polymur_test)
    polymur_seed_init(seed);
# ifdef HAVE_KHASHV
  else if(hash == khashv64_test || hash == khashv32_test)
    khashv_seed_init(seed);
# endif
#endif
  else
      return false;
  return true;
}


//-----------------------------------------------------------------------------
// Self-test on startup - verify that all installed hashes work correctly.

void SelfTest(bool verbose) {
  bool pass = true;
  for (size_t i = 0; i < sizeof(g_hashes) / sizeof(HashInfo); i++) {
    HashInfo *info = &g_hashes[i];
    if (verbose)
      printf("%20s - ", info->name);
    pass &= VerificationTest(info, verbose);
  }

  if (!pass) {
    printf("Self-test FAILED!\n");
    if (!verbose) {
      for (size_t i = 0; i < sizeof(g_hashes) / sizeof(HashInfo); i++) {
        HashInfo *info = &g_hashes[i];
        printf("%20s - ", info->name);
        pass &= VerificationTest(info, true);
      }
    }
    exit(1);
  }
}

//----------------------------------------------------------------------------

template < typename hashtype >
void test ( hashfunc<hashtype> hash, HashInfo* info )
{
  const int hashbits = sizeof(hashtype) * 8;

  if (g_testAll) {
    printf("-------------------------------------------------------------------------------\n");
  }

  // eventual initializers
  Hash_init (info);

  //-----------------------------------------------------------------------------
  // Sanity tests

  if(g_testVerifyAll)
  {
    printf("[[[ VerifyAll Tests ]]]\n\n"); fflush(NULL);
    SelfTest(g_drawDiagram);
    printf("PASS\n\n"); fflush(NULL); // if not it does exit(1)
  }

  if (g_testAll || g_testSpeed || g_testHashmap) {
    printf("--- Testing %s \"%s\" %s\n\n", info->name, info->desc, quality_str[info->quality]);
  } else {
    fprintf(stderr, "--- Testing %s \"%s\" %s\n\n", info->name, info->desc, quality_str[info->quality]);
  }
  fflush(NULL);

  // sha1_32 runs 30s
  if(g_testSanity || g_testAll)
  {
    printf("[[[ Sanity Tests ]]]\n\n");
    fflush(NULL);

    VerificationTest(info,true);
    Seed_init (info, 0);
    SanityTest(hash,hashbits);
    AppendedZeroesTest(hash,hashbits);
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Speed tests

  if(g_testSpeed || g_testAll)
  {
    double sum = 0.0;
    printf("[[[ Speed Tests ]]]\n\n");
    fflush(NULL);

    Seed_init (info, info->verification);
    BulkSpeedTest(info->hash,info->verification);
    printf("\n");
    fflush(NULL);

    for(int i = 1; i < 32; i++)
    {
      volatile int j = i;
      sum += TinySpeedTest(hashfunc<hashtype>(info->hash),sizeof(hashtype),j,info->verification,true);
    }
    g_speed = sum = sum / 31.0;
    printf("Average                                    %6.3f cycles/hash\n",sum);
    printf("\n");
    fflush(NULL);
  } else {
    // known slow hashes (> 500), cycle/hash
    const struct { pfHash h; double cycles; } speeds[] = {
     { md5_32,           670.99 },
     { md5_64,           670.99 },
     { md5_128,          730.30 },
     { sha1_32,         1385.80 },
     { sha1_64,         1385.80 },
     { sha1_160,        1470.55 },
     { sha2_224,        1354.81 },
     { sha2_224_64,     1360.10 },
     { sha2_256,        1374.90 },
     { sha2_256_64,     1376.34 },
     { rmd128,           672.35 },
     { rmd160,          1045.79 },
     { rmd256,           638.30 },
     { blake2s128_test,  698.09 },
     { blake2s160_test, 1026.74 },
     { blake2s224_test, 1063.86 },
     { blake2s256_test, 1014.88 },
     { blake2s256_64,   1014.88 },
     { blake2b160_test, 1236.84 },
     { blake2b224_test, 1228.50 },
     { blake2b256_test, 1232.22 },
     { blake2b256_64,   1236.84 },
     { sha3_256,        3877.18 },
     { sha3_256_64,     3909.00 },
     { tifuhash_64,     1679.52 },
     { floppsyhash_64,   450.93 },
     { beamsplitter_64,  682.45 },
    };
    for (int i=0; i<sizeof(speeds)/sizeof(speeds[0]); i++) {
      if (speeds[i].h == hash)
        {
          g_speed = speeds[i].cycles; break;
        }
    }
  }

  // sha1_32a runs 30s
  if(g_testHashmap || g_testAll)
  {
    printf("[[[ 'Hashmap' Speed Tests ]]]\n\n");
    fflush(NULL);
    int trials = 50;
    if ((g_speed > 500)
         && !g_testExtra)
      trials = 5;
    bool result = true;
    if (info->quality == SKIP) {
      printf("Skipping Hashmap test; it is designed for true hashes\n");
    } else {
      std::vector<std::string> words = HashMapInit(g_drawDiagram);
      if (words.size()) {
        const uint32_t seed = rand_u32();
        Seed_init (info, seed);
        result &= HashMapTest(hash,info->hashbits,words,seed,trials,g_drawDiagram);
      }
    }
    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Avalanche tests
  // 1m30 for xxh3
  // 13m  for xxh3 with --extra
  // 3m24 for xxh3 with --extra on 1 thread
  // 6m41 for xxh3 with --extra on 4 threads over bins without lock
  // 6m41 for xxh3 with --extra on 4 pinned threads
  // 3m   for farmhash128_c (was 7m with 512,1024)

  if(g_testAvalanche || g_testAll)
  {
    printf("[[[ Avalanche Tests ]]]\n\n");
    fflush(NULL);
#if NCPU_not >= 4 // 2x slower
    const bool extra = true;
#else
    const bool extra = g_testExtra;
#endif

    bool result = true;
    bool verbose = true; //.......... progress dots

    Seed_init (info, 0);
    result &= AvalancheTest< Blob< 24>, hashtype > (hash,300000,verbose);
    result &= AvalancheTest< Blob< 32>, hashtype > (hash,300000,verbose);
    result &= AvalancheTest< Blob< 40>, hashtype > (hash,300000,verbose);
    result &= AvalancheTest< Blob< 48>, hashtype > (hash,300000,verbose);
    result &= AvalancheTest< Blob< 56>, hashtype > (hash,300000,verbose);
    result &= AvalancheTest< Blob< 64>, hashtype > (hash,300000,verbose);
    result &= AvalancheTest< Blob< 72>, hashtype > (hash,300000,verbose);
    result &= AvalancheTest< Blob< 80>, hashtype > (hash,300000,verbose);

    result &= AvalancheTest< Blob< 96>, hashtype > (hash,300000,verbose);
    result &= AvalancheTest< Blob<112>, hashtype > (hash,300000,verbose);
    result &= AvalancheTest< Blob<128>, hashtype > (hash,300000,verbose);
    result &= AvalancheTest< Blob<160>, hashtype > (hash,300000,verbose);

    if(extra) {
      result &= AvalancheTest< Blob<192>, hashtype > (hash,300000,verbose);
      result &= AvalancheTest< Blob<224>, hashtype > (hash,300000,verbose);
      result &= AvalancheTest< Blob<256>, hashtype > (hash,300000,verbose);

      result &= AvalancheTest< Blob<320>, hashtype > (hash,300000,verbose);
      result &= AvalancheTest< Blob<384>, hashtype > (hash,300000,verbose);
      result &= AvalancheTest< Blob<448>, hashtype > (hash,300000,verbose);
    }
    if (extra || info->hashbits <= 64) {
      result &= AvalancheTest< Blob<512>, hashtype > (hash,300000,verbose);
    }
    if(extra) {
      result &= AvalancheTest< Blob<640>, hashtype > (hash,300000,verbose);
      result &= AvalancheTest< Blob<768>, hashtype > (hash,300000,verbose);
      result &= AvalancheTest< Blob<896>, hashtype > (hash,300000,verbose);
    }
    if (extra || info->hashbits <= 64) {
      result &= AvalancheTest< Blob<1024>,hashtype > (hash,300000,verbose);
    }
    if(extra) {
      result &= AvalancheTest< Blob<1280>,hashtype > (hash,300000,verbose);
      result &= AvalancheTest< Blob<1536>,hashtype > (hash,300000,verbose);
    }

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Keyset 'Sparse' - keys with all bits 0 except a few
  // 3m30 for xxh3
  // 14m  for xxh3 with --extra
  // 6m30 for farmhash128_c (was too much with >= 512)

  if(g_testSparse || g_testAll)
  {
    printf("[[[ Keyset 'Sparse' Tests ]]]\n\n");
    fflush(NULL);

    bool result = true;

    Seed_init (info, 0);
      result &= SparseKeyTest<  16,hashtype>(hash,9,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<  24,hashtype>(hash,8,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<  32,hashtype>(hash,7,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<  40,hashtype>(hash,6,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<  48,hashtype>(hash,6,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<  56,hashtype>(hash,5,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<  64,hashtype>(hash,5,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<  72,hashtype>(hash,5,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<  96,hashtype>(hash,4,true,true,true, g_drawDiagram);
    if (g_testExtra) {
      result &= SparseKeyTest< 112,hashtype>(hash,4,true,true,true, g_drawDiagram);
      result &= SparseKeyTest< 128,hashtype>(hash,4,true,true,true, g_drawDiagram);
      result &= SparseKeyTest< 144,hashtype>(hash,4,true,true,true, g_drawDiagram);
    }
      result &= SparseKeyTest< 160,hashtype>(hash,4,true,true,true, g_drawDiagram);
    if (g_testExtra) {
      result &= SparseKeyTest< 192,hashtype>(hash,4,true,true,true, g_drawDiagram);
    }
      result &= SparseKeyTest< 256,hashtype>(hash,3,true,true,true, g_drawDiagram);
    if (g_testExtra) {
      result &= SparseKeyTest< 288,hashtype>(hash,3,true,true,true, g_drawDiagram);
      result &= SparseKeyTest< 320,hashtype>(hash,3,true,true,true, g_drawDiagram);
      result &= SparseKeyTest< 384,hashtype>(hash,3,true,true,true, g_drawDiagram);
      result &= SparseKeyTest< 448,hashtype>(hash,3,true,true,true, g_drawDiagram);
    } else {
      if (info->hashbits > 64) //too long
        goto END_Sparse;
    }
      result &= SparseKeyTest< 512,hashtype>(hash,3,true,true,true, g_drawDiagram);
    if (g_testExtra) {
      result &= SparseKeyTest< 640,hashtype>(hash,3,true,true,true, g_drawDiagram);
      result &= SparseKeyTest< 768,hashtype>(hash,3,true,true,true, g_drawDiagram);
      result &= SparseKeyTest< 896,hashtype>(hash,2,true,true,true, g_drawDiagram);
    }
      result &= SparseKeyTest<1024,hashtype>(hash,2,true,true,true, g_drawDiagram);
    if (g_testExtra) {
      result &= SparseKeyTest<1280,hashtype>(hash,2,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<1536,hashtype>(hash,2,true,true,true, g_drawDiagram);
    }
      result &= SparseKeyTest<2048,hashtype>(hash,2,true,true,true, g_drawDiagram);
    if (g_testExtra) {
      result &= SparseKeyTest<3072,hashtype>(hash,2,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<4096,hashtype>(hash,2,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<6144,hashtype>(hash,2,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<8192,hashtype>(hash,2,true,true,true, g_drawDiagram);
      result &= SparseKeyTest<9992,hashtype>(hash,2,true,true,true, g_drawDiagram);
    }
  END_Sparse:
    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Keyset 'Permutation' - all possible combinations of a set of blocks
  // 9m with xxh3 and maxlen=23, 4m15 with maxlen=22
  // 120m for farmhash128_c with maxlen=18, 1m20 FAIL with maxlen=12
  //                                        1m20 PASS with maxlen=14,16,17

  if(g_testPermutation || g_testAll)
  {
    const int maxlen = g_testExtra
      ? 23
      : info->hashbits > 64
         ? 17
         : 22;

    {
      // This one breaks lookup3, surprisingly
      printf("[[[ Keyset 'Permutation' Tests ]]]\n\n");
      printf("Combination Lowbits Tests:\n");
      fflush(NULL);

      bool result = true;
      uint32_t blocks[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

      Seed_init (info, 0);
      result &= CombinationKeyTest<hashtype>(hash,7,blocks,
                                             sizeof(blocks) / sizeof(uint32_t),
                                             true,true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination Highbits Tests\n");
      fflush(NULL);

      bool result = true;

      uint32_t blocks[] =
      {
        0x00000000,
        0x20000000, 0x40000000, 0x60000000, 0x80000000, 0xA0000000, 0xC0000000, 0xE0000000
      };

      result &= CombinationKeyTest(hash,7,blocks,sizeof(blocks) / sizeof(uint32_t),
                                   true,true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination Hi-Lo Tests:\n");

      bool result = true;

      uint32_t blocks[] =
      {
        0x00000000,
        0x00000001, 0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000006, 0x00000007,
        0x80000000, 0x40000000, 0xC0000000, 0x20000000, 0xA0000000, 0x60000000, 0xE0000000
      };

      result &= CombinationKeyTest<hashtype>(hash,6,blocks,sizeof(blocks) / sizeof(uint32_t),
                                             true,true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 0x8000000 Tests:\n");
      fflush(NULL);

      bool result = true;

      uint32_t blocks[] =
      {
        0x00000000,
        0x80000000,
      };

      result &= CombinationKeyTest(hash, maxlen, blocks, sizeof(blocks) / sizeof(uint32_t),
                                   true,true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 0x0000001 Tests:\n");

      bool result = true;

      uint32_t blocks[] =
      {
        0x00000000,
        0x00000001,
      };

      result &= CombinationKeyTest(hash, maxlen, blocks, sizeof(blocks) / sizeof(uint32_t),
                                   true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 0x800000000000000 Tests:\n");
      fflush(NULL);

      bool result = true;

      uint64_t blocks[] =
      {
        0x0000000000000000ULL,
        0x8000000000000000ULL,
      };

      result &= CombinationKeyTest(hash, maxlen, blocks, sizeof(blocks) / sizeof(uint64_t),
                                   true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 0x000000000000001 Tests:\n");
      fflush(NULL);

      bool result = true;

      uint64_t blocks[] =
      {
        0x0000000000000000ULL,
        0x0000000000000001ULL,
      };

      result &= CombinationKeyTest(hash, maxlen, blocks, sizeof(blocks) / sizeof(uint64_t),
                                   true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 16-bytes [0-1] Tests:\n");
      fflush(NULL);

      bool result = true;

      block16 blocks[2];
      memset(blocks, 0, sizeof(blocks));
      blocks[0].c[0] = 1;   // presumes little endian

      result &= CombinationKeyTest(hash, maxlen, blocks, 2, true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 16-bytes [0-last] Tests:\n");
      fflush(NULL);

      bool result = true;

      const size_t nbElts = 2;
      block16 blocks[nbElts];
      memset(blocks, 0, sizeof(blocks));
      blocks[0].c[sizeof(blocks[0].c)-1] = 0x80;   // presumes little endian

      result &= CombinationKeyTest(hash, maxlen, blocks, nbElts, true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 32-bytes [0-1] Tests:\n");
      fflush(NULL);

      bool result = true;

      block32 blocks[2];
      memset(blocks, 0, sizeof(blocks));
      blocks[0].c[0] = 1;   // presumes little endian

      result &= CombinationKeyTest(hash, maxlen, blocks, 2, true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 32-bytes [0-last] Tests:\n");
      fflush(NULL);

      bool result = true;

      size_t const nbElts = 2;
      block32 blocks[nbElts];
      memset(blocks, 0, sizeof(blocks));
      blocks[0].c[sizeof(blocks[0].c)-1] = 0x80;   // presumes little endian

      result &= CombinationKeyTest(hash, maxlen, blocks, nbElts, true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 64-bytes [0-1] Tests:\n");
      fflush(NULL);

      bool result = true;

      block64 blocks[2];
      memset(blocks, 0, sizeof(blocks));
      blocks[0].c[0] = 1;   // presumes little endian

      result &= CombinationKeyTest(hash, maxlen, blocks, 2, true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 64-bytes [0-last] Tests:\n");
      fflush(NULL);

      bool result = true;

      size_t const nbElts = 2;
      block64 blocks[nbElts];
      memset(blocks, 0, sizeof(blocks));
      blocks[0].c[sizeof(blocks[0].c)-1] = 0x80;   // presumes little endian

      result &= CombinationKeyTest(hash, maxlen, blocks, nbElts, true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 128-bytes [0-1] Tests:\n");
      fflush(NULL);

      bool result = true;

      block128 blocks[2];
      memset(blocks, 0, sizeof(blocks));
      blocks[0].c[0] = 1;   // presumes little endian

      result &= CombinationKeyTest(hash, maxlen, blocks, 2, true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

    {
      printf("Combination 128-bytes [0-last] Tests:\n");
      fflush(NULL);

      bool result = true;

      size_t const nbElts = 2;
      block128 blocks[nbElts];
      memset(blocks, 0, sizeof(blocks));
      blocks[0].c[sizeof(blocks[0].c)-1] = 0x80;   // presumes little endian

      result &= CombinationKeyTest(hash, maxlen, blocks, nbElts, true, true, g_drawDiagram);

      if(!result) printf("*********FAIL*********\n");
      printf("\n");
      fflush(NULL);
    }

  }

  //-----------------------------------------------------------------------------
  // Keyset 'Window'

  // Skip distribution test for these - they're too easy to distribute well,
  // and it generates a _lot_ of testing.
  // 11s for crc32_hw, 28s for xxh3
  // 51s for crc32_hw --extra
  // 180m for farmhash128_c with 20 windowbits,
  //      0.19s with windowbits=10, 2s for 14, 9s for 16, 37s for 18
  // 7m for FNV64 with windowbits=27 / 32bit keys
  // 5m35 for hasshe2 with windowbits=25 / 32bit keys

  if((g_testWindow || g_testAll) && !need_minlen64_align16(hash))
  {
    printf("[[[ Keyset 'Window' Tests ]]]\n\n");

    bool result = true;
    bool testCollision = true;
    bool testDistribution = g_testExtra;
    // This value is now adjusted to generate at least 0.5 collisions per window,
    // except for 64++bit where it unrealistic. There use smaller but more keys,
    // to get a higher collision percentage.
    int windowbits = 20;
    const int keybits = (hashbits >= 64) ? 32 : hashbits*2+2;

    Seed_init (info, 0);
    result &= WindowedKeyTest< Blob<keybits>, hashtype >
        ( hash, windowbits, testCollision, testDistribution, g_drawDiagram );

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Keyset 'Cyclic' - keys of the form "abcdabcdabcd..."
  // 5s for crc32_hw
  // 18s for farmhash128_c

  if ((g_testCyclic || g_testAll) && !need_minlen64_align16(hash))
  {
    printf("[[[ Keyset 'Cyclic' Tests ]]]\n\n");
    fflush(NULL);
#ifdef DEBUG
    const int reps = 2;
#else
    const int reps = g_speed > 500.0 ? 100000 : 1000000;
#endif
    bool result = true;

    Seed_init (info, 0);
    result &= CyclicKeyTest<hashtype>(hash,sizeof(hashtype)+0,8,reps, g_drawDiagram);
    result &= CyclicKeyTest<hashtype>(hash,sizeof(hashtype)+1,8,reps, g_drawDiagram);
    result &= CyclicKeyTest<hashtype>(hash,sizeof(hashtype)+2,8,reps, g_drawDiagram);
    result &= CyclicKeyTest<hashtype>(hash,sizeof(hashtype)+3,8,reps, g_drawDiagram);
    result &= CyclicKeyTest<hashtype>(hash,sizeof(hashtype)+4,8,reps, g_drawDiagram);
    result &= CyclicKeyTest<hashtype>(hash,sizeof(hashtype)+8,8,reps, g_drawDiagram);

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Keyset 'TwoBytes' - all keys up to N bytes containing two non-zero bytes
  // 3m40 for crc32_hw (32bit), 8m30 for xxh3 --extra (64bit)
  // 4m16 for xxh3
  // 4m50 for metrohash128crc_1
  // 260m for farmhash128_c with maxlen=16, 31s with maxlen=10, 2m with 12,14,15

  // With --extra this generates some huge keysets,
  // 128-bit tests will take ~1.3 gigs of RAM.

  if(g_testTwoBytes || g_testAll)
  {
    printf("[[[ Keyset 'TwoBytes' Tests ]]]\n\n");
    fflush(NULL);

    bool result = true;
    int maxlen = 24;
    if (!g_testExtra && (info->hashbits > 32)) {
      maxlen = (info->hashbits < 128) ? 20 : 15;
      if (g_speed > 500.0)
        maxlen = 8;
    }

    Seed_init (info, 0);
    for(int len = 4; len <= maxlen; len += 4)
    {
      result &= TwoBytesTest2<hashtype>(hash, len, g_drawDiagram);
    }

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Keyset 'Text'

  if(g_testText || g_testAll)
  {
    printf("[[[ Keyset 'Text' Tests ]]]\n\n");

    bool result = true;

    const char * alnum = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const char * passwordchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
                                 ".,!?:;-+=()<>/|\"'@#$%&*_^";

    Seed_init (info, 0);
    result &= TextKeyTest( hash, "Foo",    alnum, 4, "Bar",    g_drawDiagram );
    result &= TextKeyTest( hash, "FooBar", alnum, 4, "",       g_drawDiagram );
    result &= TextKeyTest( hash, "",       alnum, 4, "FooBar", g_drawDiagram );

    // maybe use random-len vector of strings here, from len 6-16
    result &= WordsKeyTest( hash, 4000000L, 6, 16, alnum, "alnum", g_drawDiagram );
    result &= WordsKeyTest( hash, 4000000L, 6, 16, passwordchars, "password", g_drawDiagram );
    std::vector<std::string> words = HashMapInit(g_drawDiagram);
    result &= WordsStringTest( hash, words, g_drawDiagram );

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Keyset 'Zeroes'

  if(g_testZeroes || g_testAll)
  {
    printf("[[[ Keyset 'Zeroes' Tests ]]]\n\n");

    bool result = true;

    Seed_init (info, 0);
    result &= ZeroKeyTest<hashtype>( hash, g_drawDiagram );

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Keyset 'Seed'

  if(g_testSeed || g_testAll)
  {
    printf("[[[ Keyset 'Seed' Tests ]]]\n\n");

    bool result = true;

    result &= SeedTest<hashtype>( hash, 5000000U, g_drawDiagram );

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Keyset 'PerlinNoise'

  if(g_testPerlinNoise || g_testAll)
  {
    printf("[[[ Keyset 'PerlinNoise' Tests ]]]\n\n");

    bool testCollision = true;
    bool testDistribution = g_testExtra;

    bool result = true;
    Seed_init (info, 0);
    result &= PerlinNoise<hashtype>( hash, 2, testCollision, testDistribution, g_drawDiagram );
    if (g_testExtra) {
        result &= PerlinNoise<hashtype>( hash, 4, testCollision, testDistribution, g_drawDiagram );
        result &= PerlinNoise<hashtype>( hash, 8, testCollision, testDistribution, g_drawDiagram );
    }
    result &= PerlinNoiseAV<hashtype>( hash, testCollision, testDistribution, g_drawDiagram );

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }


  //-----------------------------------------------------------------------------
  // Differential tests
  // 5m30 with xxh3
  // less reps with slow or very bad hashes
  // md5: 1h38m with 1000 reps!
  // halftime* > 40m

  if(g_testDiff || g_testAll)
  {
    printf("[[[ Diff 'Differential' Tests ]]]\n\n");
    fflush(NULL);

    bool result = true;
    bool dumpCollisions = g_drawDiagram; // from --verbose
    int reps = 1000;
    if ((g_speed > 500.0 || info->hashbits > 128 ||
         hash == o1hash_test ||
         hash == halftime_hash_style64_test ||
         hash == halftime_hash_style128_test ||
         hash == halftime_hash_style256_test ||
         hash == halftime_hash_style512_test
         ) && !g_testExtra)
      reps = 100; // sha1: 7m, md5: 4m53

    result &= DiffTest< Blob<64>,  hashtype >(hash,5,reps,dumpCollisions);
    result &= DiffTest< Blob<128>, hashtype >(hash,4,reps,dumpCollisions);
    result &= DiffTest< Blob<256>, hashtype >(hash,3,reps,dumpCollisions);

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // Differential-distribution tests
  // 2m40 with xxh3

  if (g_testDiffDist || g_testAll)
  {
    printf("[[[ DiffDist 'Differential Distribution' Tests ]]]\n\n");
    fflush(NULL);

    bool result = true;

    result &= DiffDistTest2<uint64_t,hashtype>(hash, g_drawDiagram);

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  // Moment Chi-Square test, measuring the probability of the
  // lowest 32 bits set over the whole key space. Not where the bits are, but how many.
  // See e.g. https://www.statlect.com/fundamentals-of-probability/moment-generating-function
  // 10s (16 step interval until 0x7ffffff)
  // 20s (16 step interval until 0xcffffff)
  //   step  time
  //   1     300s
  //   2     150s
  //   3     90s
  //   7     35s
  //   13    20s
  //   16    12s
  if (g_testMomentChi2 || g_testAll)
  {
    printf("[[[ MomentChi2 Tests ]]]\n\n");

    bool result = true;
    result &= MomentChi2Test(info, 4);
    if (g_testExtra) {
        result &= MomentChi2Test(info, 8);
        result &= MomentChi2Test(info, 16);
    }

    if(!result) printf("\n*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  if (g_testPrng || g_testAll)
  {
    printf("[[[ Prng Tests ]]]\n\n");

    bool testCollision = true;
    bool testDistribution = g_testExtra;

    bool result = true;
    Seed_init (info, 0);
    result &= PrngTest<hashtype>( hash, testCollision, testDistribution, g_drawDiagram );

    if(!result) printf("\n*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  //-----------------------------------------------------------------------------
  // LongNeighbors - collisions between long messages of low Hamming distance
  // esp. for testing separate word and then byte-wise processing of unaligned
  // rest parts. Only with --test=LongNeighbors or --extra
  // 10s for fasthash32
  // 7m with xxh3 (64bit)
  // 10m30s with farmhash128_c

  // Not yet included for licensing reasons
#if 0
  if(g_testLongNeighbors || (g_testAll && g_testExtra))
  {
    printf("[[[ LongNeighbors Tests ]]]\n\n");

    bool result = true;

    Seed_init (info, 0);
    result &= testLongNeighbors(info->hash, info->hashbits, g_drawDiagram);

    if(!result) printf("*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }
#endif

  //-----------------------------------------------------------------------------
  // Bit Independence Criteria. Interesting, but doesn't tell us much about
  // collision or distribution. For 128bit hashes only with --extra
  // 4m with xxh3
  // 152m with farmhash128_c with reps=1000000, => 8m with 100000

  if(g_testBIC || (g_testAll && info->hashbits > 64 && g_testExtra))
  {
    printf("[[[ BIC 'Bit Independence Criteria' Tests ]]]\n\n");
    fflush(NULL);

    bool result = true;
    Seed_init (info, 0);
    if (info->hashbits > 64 || g_speed > 500.0) {
      result &= BicTest3<Blob<128>,hashtype>(hash,100000,g_drawDiagram);
    } else {
      const long reps = 64000000/info->hashbits;
      //result &= BicTest<uint64_t,hashtype>(hash,2000000);
      result &= BicTest3<Blob<88>,hashtype>(hash,(int)reps,g_drawDiagram);
    }

    if(!result) printf("\n*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }

  if (g_testBadSeeds || g_testAll)
  {
    printf("[[[ BadSeeds Tests ]]]\n\n");
    // g_testExtra: test all seeds. if not just some known secrets/bad seeds

    Seed_init (info, 0);
    bool result = BadSeedsTest<hashtype>( info, g_testExtra );
    if(!result) printf("\n*********FAIL*********\n");
    printf("\n");
    fflush(NULL);
  }
  
}

//-----------------------------------------------------------------------------

uint32_t g_inputVCode = 1;
uint32_t g_outputVCode = 1;
uint32_t g_resultVCode = 1;

HashInfo * g_hashUnderTest = NULL;

void VerifyHash ( const void * key, int len, uint32_t seed, void * out )
{
  g_inputVCode = MurmurOAAT((const char *)key, len, g_inputVCode);
  g_inputVCode = MurmurOAAT((const char *)&seed, sizeof(uint32_t), g_inputVCode);

  g_hashUnderTest->hash(key, len, seed, out);

  g_outputVCode = MurmurOAAT((const char *)out, g_hashUnderTest->hashbits/8, g_outputVCode);
}

typedef long double moments[8];

// Copy the results into NCPU ranges of 2^32
void MomentChi2Thread ( const struct HashInfo *info, const int inputSize,
                        const unsigned start, const unsigned end, const unsigned step,
                        moments &b)
{
  pfHash const hash = info->hash;
  uint32_t seed = 0;
  long double const n = (end-(start+1)) / step;
  uint64_t previous = 0;
  long double b0h = b[0], b0l = b[1], db0h = b[2], db0l = b[3];
  long double b1h = b[4], b1l = b[5], db1h = b[6], db1l = b[7];
#define INPUT_SIZE_MAX 256
  assert(inputSize <= INPUT_SIZE_MAX);
  char key[INPUT_SIZE_MAX] = {0};
#define HASH_SIZE_MAX 64
  char hbuff[HASH_SIZE_MAX] = {0};
  int hbits = info->hashbits;  
  if (hbits > 64) hbits = 64;   // limited due to popcount8
  Bad_Seed_init(hash, seed);
  Hash_Seed_init(hash, seed);
  assert(sizeof(unsigned) <= inputSize);
  assert(start < end);
  //assert(step > 0);

  uint64_t i = start - step;
  memcpy(key, &i, sizeof(i));
  hash(key, inputSize, seed, hbuff);
  memcpy(&previous, hbuff, 8);

  for (uint64_t i=start; i<=end; i+=step) {
    memcpy(key, &i, sizeof(i));
    hash(key, inputSize, seed, hbuff);

    uint64_t h; memcpy(&h, hbuff, 8);
    // popcount8 assumed to work on 64-bit
    // note : ideally, one should rather popcount the whole hash
    {
      uint64_t const bits1 = popcount8(h);
      uint64_t const bits0 = hbits - bits1;
      uint64_t const b1_exp5 = bits1 * bits1 * bits1 * bits1 * bits1;
      uint64_t const b0_exp5 = bits0 * bits0 * bits0 * bits0 * bits0;
      b1h += b1_exp5; b1l += b1_exp5 * b1_exp5;
      b0h += b0_exp5; b0l += b0_exp5 * b0_exp5;
    }
    // derivative
    {
      uint64_t const bits1 = popcount8(previous^h);
      uint64_t const bits0 = hbits - bits1;
      uint64_t const b1_exp5 = bits1 * bits1 * bits1 * bits1 * bits1;
      uint64_t const b0_exp5 = bits0 * bits0 * bits0 * bits0 * bits0;
      db1h += b1_exp5; db1l += b1_exp5 * b1_exp5;
      db0h += b0_exp5; db0l += b0_exp5 * b0_exp5;
    }
    previous = h;
  }

  b[0] = b0h;
  b[1] = b0l;
  b[2] = db0h;
  b[3] = db0l;
  b[4] = b1h;
  b[5] = b1l;
  b[6] = db1h;
  b[7] = db1l;
}

// sha1_32a: 23m with step 3
//           4m30 with step 2, 4 threads, ryzen3
bool MomentChi2Test ( struct HashInfo *info, int inputSize)
{
  const pfHash hash = info->hash;
  const int step = ((g_speed > 500 || info->hashbits > 128)
                    && !g_testExtra) ? 6 : 2;
  const unsigned mx = 0xffffffff;
  assert(inputSize >= 4);
  long double const n = 0x100000000UL / step;
  int hbits = info->hashbits;
  if (hbits > 64) hbits = 64;   // limited due to popcount8
  assert(hbits <= HASH_SIZE_MAX*8);
  assert(inputSize > 0);

  printf("Analyze hashes produced from a serie of linearly increasing numbers "
         "of %i-bit, using a step of %d ... \n", inputSize*8, step);
  fflush(NULL);

  /* Notes on the ranking system.
   * Ideally, this test should report and sum all popcount values
   * and compare the resulting distribution to an ideal distribution.
   *
   * What happens here is quite simplified :
   * the test gives "points" for each popcount, and sum them all.
   * The metric (using N^5) is heavily influenced by the largest outliers.
   * For example, a 64-bit hash should have a popcount close to 32.
   * But a popcount==40 will tilt the metric upward
   * more than popcount==24 will tilt the metric downward.
   * In reality, both situations should be ranked similarly.
   *
   * To compensate, we measure both popcount1 and popcount0,
   * and compare to some pre-calculated "optimal" sums for the hash size.
   *
   * Another limitation of this test is that it only popcounts the first 64-bit.
   * For large hashes, bits beyond this limit are ignored.
   */

  long double srefh, srefl;
  switch (hbits/8) {
      case 8:
          srefh = 38918200.;
          if (step == 2)
            srefl = 273633.333333;
          else if (step == 6)
            srefl = 820900.0;
          else
            abort();
          break;
      case 4:
          srefh = 1391290.;
          if (step == 2)
            srefl = 686.6666667;
          else if (step == 6)
            srefl = 2060.0;
          else
            abort();
          break;
      default:
          printf("hash size not covered \n");
          abort();
  }
  printf("Target values to approximate : %Lf - %Lf \n", srefh, srefl);

#if NCPU > 1
  // split into NCPU threads
  const uint64_t len = 0x100000000UL / NCPU;
  moments b[NCPU];
  static std::thread t[NCPU];
  printf("%d threads starting... ", NCPU);
  fflush(NULL);
  for (int i=0; i < NCPU; i++) {
    const unsigned start = i * len;
    b[i][0] = 0.; b[i][1] = 0.; b[i][2] = 0.; b[i][3] = 0.;
    b[i][4] = 0.; b[i][5] = 0.; b[i][6] = 0.; b[i][7] = 0.;
    //printf("thread[%d]: %d, 0x%x - 0x%x %d\n", i, inputSize, start, start + len - 1, step);
    t[i] = std::thread {MomentChi2Thread, info, inputSize, start, start + (len - 1), step, std::ref(b[i])};
    // pin it? moves around a lot. but the result is fair
  }
  fflush(NULL);
  std::this_thread::sleep_for(std::chrono::seconds(5));
  for (int i=0; i < NCPU; i++) {
    t[i].join();
  }
  printf(" done\n");
  //printf("[%d]: %Lf, %Lf, %Lf, %Lf, %Lf, %Lf, %Lf, %Lf\n", 0,
  //       b[0][0], b[0][1], b[0][2], b[0][3], b[0][4], b[0][5], b[0][6], b[0][7]);
  for (int i=1; i < NCPU; i++) {
    //printf("[%d]: %Lf, %Lf, %Lf, %Lf, %Lf, %Lf, %Lf, %Lf\n", i,
    //       b[i][0], b[i][1], b[i][2], b[i][3], b[i][4], b[i][5], b[i][6], b[i][7]);
    for (int j=0; j < 8; j++)
      b[0][j] += b[i][j];
  }

  long double b0h = b[0][0], b0l = b[0][1], db0h = b[0][2], db0l = b[0][3];
  long double b1h = b[0][4], b1l = b[0][5], db1h = b[0][6], db1l = b[0][7];

#else  

  moments b = {0.,0.,0.,0.,0.,0.,0.,0.};
  MomentChi2Thread (info, inputSize, 0, 0xffffffff, step, b);

  long double b0h = b[0], b0l = b[1], db0h = b[2], db0l = b[3];
  long double b1h = b[4], b1l = b[5], db1h = b[6], db1l = b[7];

#endif
  
  b1h  /= n;  b1l = (b1l/n  - b1h*b1h) / n;
  db1h /= n; db1l = (db1l/n - db1h*db1h) / n;
  b0h  /= n;  b0l = (b0l/n  - b0h*b0h) / n;
  db0h /= n; db0l = (db0l/n - db0h*db0h) / n;

  printf("Popcount 1 stats : %Lf - %Lf\n", b1h, b1l);
  printf("Popcount 0 stats : %Lf - %Lf\n", b0h, b0l);
  double worsec2 = 0;
  {   double chi2 = (b1h-srefh) * (b1h-srefh) / (b1l+srefl);
      printf("MomentChi2 for bits 1 :  %8.6g \n", chi2);
      if (chi2 > worsec2) worsec2 = chi2;
  }
  {   double chi2 = (b0h-srefh) * (b0h-srefh) / (b0l+srefl);
      printf("MomentChi2 for bits 0 :  %8.6g \n", chi2);
      if (chi2 > worsec2) worsec2 = chi2;
  }

  /* Derivative :
   * In this scenario, 2 consecutive hashes are xored,
   * and the outcome of this xor operation is then popcount controlled.
   * Obviously, the _order_ in which the hash values are generated becomes critical.
   *
   * This scenario comes from the prng world,
   * where derivative of the generated suite of random numbers is analyzed
   * to ensure the suite is truly "random".
   *
   * However, in almost all prng, the seed of next random number is the previous random number.
   *
   * This scenario is quite different: it introduces a fixed distance between 2 consecutive "seeds".
   * This is especially detrimental to algorithms relying on linear operations, such as multiplications.
   *
   * This scenario is relevant if the hash is used as a prng and generates values from a linearly increasing counter as a seed.
   * It is not relevant for scenarios employing the hash as a prng
   * with the more classical method of using the previous random number as a seed for the next one.
   * This scenario has no relevance for classical usages of hash algorithms,
   * such as hash tables, bloom filters and such, were only the raw values are ever used.
   */
  printf("\nDerivative stats (transition from 2 consecutive values) : \n");
  printf("Popcount 1 stats : %Lf - %Lf\n", db1h, db1l);
  printf("Popcount 0 stats : %Lf - %Lf\n", db0h, db0l);
  {   double chi2 = (db1h-srefh) * (db1h-srefh) / (db1l+srefl);
      printf("MomentChi2 for deriv b1 :  %8.6g \n", chi2);
      if (chi2 > worsec2) worsec2 = chi2;
  }
  {   double chi2 = (db0h-srefh) * (db0h-srefh) / (db0l+srefl);
      printf("MomentChi2 for deriv b0 :  %8.6g \n", chi2);
      if (chi2 > worsec2) worsec2 = chi2;
  }

  // note : previous threshold : 3.84145882069413
  int const rank = (worsec2 < 500.) + (worsec2 < 50.) + (worsec2 < 5.);
  assert(0 <= rank && rank <= 3);

  const char* rankstr[4] = { "FAIL !!!!", "pass", "Good", "Great" };
  printf("\n  %s \n\n", rankstr[rank]);
  fflush(NULL);

  return (rank > 0);
}


//-----------------------------------------------------------------------------

void testHash ( const char * name )
{
  HashInfo * pInfo = findHash(name);

  if(pInfo == NULL)
  {
    printf("Invalid hash '%s' specified\n", name);
    return;
  }
  else
  {
    g_hashUnderTest = pInfo;

    if(pInfo->hashbits == 32)
    {
      test<uint32_t>( pInfo->hash, pInfo );
    }
    else if(pInfo->hashbits == 64)
    {
      test<uint64_t>( pInfo->hash, pInfo );
    }
    else if(pInfo->hashbits == 128)
    {
      test<uint128_t>( pInfo->hash, pInfo );
    }
    else if(pInfo->hashbits == 160)
    {
      test<Blob<160>>( pInfo->hash, pInfo );
    }
    else if(pInfo->hashbits == 224)
    {
      test<Blob<224>>( pInfo->hash, pInfo );
    }
    else if(pInfo->hashbits == 256)
    {
      test<uint256_t>( pInfo->hash, pInfo );
    }
    else
    {
      printf("Invalid hash bit width %d for hash '%s'",
             pInfo->hashbits, pInfo->name);
    }
  }
}
//-----------------------------------------------------------------------------

/*
 * This list of actual expected collision values was generated via the
 * exactcoll.c program which uses the MPFI or MPFR library to compute
 * these values with 768 bits of precision, and then post-processed
 * via strtod() to get the maximum number of digits that can fit in a
 * double.
 */
double realcoll[58][18] = {
    /* 149633745 */
    { 9.66830188511513408e-62, 4.15250404044246501e-52, 7.66001792990870096e-33,
      3.28995264957314909e-23, 6.06889145411344312e-04, 3.10727242021280714e-01,
      3.18184245207177412e+02, 2.54544870233834445e+03, 2.03619731305636706e+04,
      1.62792385217456205e+05, 2.57656049031511368e+06, 1.90430490019698478e+07,
      5.94342984822125658e+07, 1.32858774460385174e+08, 1.45439441000000000e+08,
      1.49109457000000000e+08, 1.49629649000000000e+08, 1.49633489000000000e+08 },
    /* 86536545 */
    { 3.23362916384237121e-62, 1.38883315060948101e-52, 2.56194496903768089e-33,
      1.10034698561685720e-23, 2.02978192359201898e-04, 1.03924834404869174e-01,
      1.06418943269388180e+02, 8.51346660380768071e+02, 6.81046060560096157e+03,
      5.44636796883101269e+04, 8.65959061394601478e+05, 6.61418293104189448e+06,
      2.27556140267314911e+07, 6.98558535013311207e+07, 8.23422410045954734e+07,
      8.60122570000000000e+07, 8.65324490000000000e+07, 8.65362890000000000e+07 },
    /* 75498113 */
    { 2.46129292104772484e-62, 1.05711726017762883e-52, 1.95003715543977527e-33,
      8.37534580859870329e-24, 1.54497860659825494e-04, 7.91029046026853616e-02,
      8.10013164325720538e+01, 6.48007286993706316e+02, 5.18385065708740240e+03,
      4.14575199616562895e+04, 6.59692186580697889e+05, 5.06817564395631664e+06,
      1.77549757986361682e+07, 5.89072678887400925e+07, 7.13038090638692677e+07,
      7.49738250000000000e+07, 7.54940170000000000e+07, 7.54978570000000000e+07 },
    /* 56050289 */
    { 1.35658440124283578e-62, 5.82648563760172142e-53, 1.07479689405983373e-33,
      4.61621750982936253e-24, 8.51541829923128089e-05, 4.35989416694992429e-02,
      4.46452925853961631e+01, 3.57161013077325094e+02, 2.85720313997638277e+03,
      2.28521884740198511e+04, 3.64148636055323470e+05, 2.82665629721443821e+06,
      1.02311598958176058e+07, 3.98670968021314815e+07, 5.18559915916659608e+07,
      5.55260010000000000e+07, 5.60461930000000000e+07, 5.60500330000000000e+07 },
    /* 49925029 */
    { 1.07628616390943998e-62, 4.62261387512834023e-53, 8.52721751060712554e-34,
      3.66241203339361373e-24, 6.75595774724252468e-05, 3.45905036499356000e-02,
      3.54206590004570572e+01, 2.83364333813803171e+02, 2.26685462770169033e+03,
      1.81309949687949847e+04, 2.89045130868813896e+05, 2.25101610920316912e+06,
      8.23359498302312009e+06, 3.40035930111785606e+07, 4.57307533941198885e+07,
      4.94007410000000000e+07, 4.99209330000000000e+07, 4.99247730000000000e+07 },
    /* 44251425 */
    { 8.45562327779528750e-63, 3.63166254454270828e-53, 6.69923495212561545e-34,
      2.87729950275996440e-24, 5.30768075507823733e-05, 2.71753254548965095e-02,
      2.78275216109708978e+01, 2.22619519580197675e+02, 1.78091434578536018e+03,
      1.42446392954819730e+04, 2.27182256963651860e+05, 1.77461480911257491e+06,
      6.55507402957992628e+06, 2.86743406137902029e+07, 4.00572308235341832e+07,
      4.37271370000000000e+07, 4.42473290000000000e+07, 4.42511690000000000e+07 },
    /* 43691201 */
    { 8.24288176206433810e-63, 3.54029075928611856e-53, 6.53068375830698963e-34,
      2.80490731624468888e-24, 5.17414074132004304e-05, 2.64916005848709717e-02,
      2.71273877811360791e+01, 2.17018473441357912e+02, 1.73610754462317163e+03,
      1.38862852138241597e+04, 2.21476017148987623e+05, 1.73055958502948540e+06,
      6.39857166559864674e+06, 2.81548679497163482e+07, 3.94970225171834230e+07,
      4.31669130000000000e+07, 4.36871050000000000e+07, 4.36909450000000000e+07 },
    /* 33558529 */
    { 4.86291784915122170e-63, 2.08860731252391586e-53, 3.85280045646069782e-34,
      1.65476519585125690e-24, 3.05250300699314860e-05, 1.56288153909619858e-02,
      1.60039018771892643e+01, 1.28030930083075560e+02, 1.02422920513447593e+03,
      8.19266670739054098e+03, 1.30763213462519823e+05, 1.02731598739112553e+06,
      3.86648187299589021e+06, 1.90513077430028245e+07, 2.93656306571820080e+07,
      3.30342410000000000e+07, 3.35544330000000000e+07, 3.35582730000000000e+07 },
    /* 33554432 */
    { 4.86173054093815170e-63, 2.08809736752937507e-53, 3.85185977398010151e-34,
      1.65436117580224877e-24, 3.05175772154867956e-05, 1.56249995294880754e-02,
      1.59999944369014884e+01, 1.27999670665119382e+02, 1.02397913646883865e+03,
      8.19066658538974480e+03, 1.30731328417170167e+05, 1.02706774802737299e+06,
      3.86557557111472497e+06, 1.90477651439465471e+07, 2.93615350309002101e+07,
      3.30301440000000000e+07, 3.35503360000000000e+07, 3.35541760000000000e+07 },
    /* 26977161 */
    { 3.14256005499304537e-63, 1.34971926619110914e-53, 2.48979258747824472e-34,
      1.06935777370422802e-24, 1.97261691747440925e-05, 1.00997986149531007e-02,
      1.03421911410463228e+01, 8.27373811067683533e+01, 6.61889575586005321e+02,
      5.29451037409544824e+03, 8.45461443414444802e+04, 6.66574543746769894e+05,
      2.53827383658029372e+06, 1.35603369840820655e+07, 2.27896075604615994e+07,
      2.64528730000000000e+07, 2.69730650000000000e+07, 2.69769050000000000e+07 },
    /* 22370049 */
    { 2.16085171788696973e-63, 9.28078745982995323e-54, 1.71200311073976113e-34,
      7.35299737127754043e-25, 1.35638860682561044e-05, 6.94470966551262447e-03,
      7.11138119182984063e+00, 5.68909651356401653e+01, 4.55122319603302856e+02,
      3.64063288968196957e+03, 5.81554370404469810e+04, 4.59645385789985245e+05,
      1.76481282635707408e+06, 1.00151462171464767e+07, 1.81959928124494441e+07,
      2.18457610000000000e+07, 2.23659530000000000e+07, 2.23697930000000000e+07 },
    /* 18877441 */
    { 1.53878283990836292e-63, 6.60902197305242237e-54, 1.21914936914420980e-34,
      5.23620666941341261e-25, 9.65909643476873488e-06, 4.94545737373954832e-03,
      5.06414744590625077e+00, 4.05131288488040155e+01, 3.24101784837318064e+02,
      2.59260655174234762e+03, 4.14247903550759002e+04, 3.28028082683300890e+05,
      1.26742600458991365e+06, 7.54599182152087614e+06, 1.47296973581916802e+07,
      1.83531530000000000e+07, 1.88733450000000000e+07, 1.88771850000000000e+07 },
    /* 18616785 */
    { 1.49658179329122305e-63, 6.42776985797483522e-54, 1.18571425534766178e-34,
      5.09260394911920045e-25, 9.39419617181328754e-06, 4.80982843914157677e-03,
      4.92526345384282216e+00, 3.94020589843511928e+01, 3.15213358531706945e+02,
      2.52150762757849679e+03, 4.02895318773614636e+04, 3.19083263398166222e+05,
      1.23344671390196425e+06, 7.37060359433948807e+06, 1.44720266633904669e+07,
      1.80924970000000000e+07, 1.86126890000000000e+07, 1.86165290000000000e+07 },
    /* 17676661 */
    { 1.34924729526152486e-63, 5.79497300736470505e-54, 1.06898383980911691e-34,
      4.59125063193266000e-25, 8.46936253854919755e-06, 4.33631361902940549e-03,
      4.44038440299461268e+00, 3.55230335814082565e+01, 2.84181603549241117e+02,
      2.27328227266108661e+03, 3.63257830806934944e+04, 2.87837384102243173e+05,
      1.11455845455760439e+06, 6.74926355401089974e+06, 1.35443510115238819e+07,
      1.71523730000000000e+07, 1.76725650000000000e+07, 1.76764050000000000e+07 },
    /* 16777216 */
    { 1.21543259901182161e-63, 5.22024326324805573e-54, 9.62964914796432828e-35,
      4.13590281624610549e-25, 7.62939407650033587e-06, 3.90624976656302669e-03,
      3.99999912579873262e+00, 3.19999574025932816e+01, 2.55997380594878024e+02,
      2.04783322146484898e+03, 3.27253730219586105e+04, 2.59434518880420335e+05,
      1.00621717678566615e+06, 6.17199266255285591e+06, 1.26597333208222985e+07,
      1.62529280000000075e+07, 1.67731200000000000e+07, 1.67769600000000000e+07 },
    /* 16777214 */
    { 1.21543230923011700e-63, 5.22024201864511143e-54, 9.62964685207712960e-35,
      4.13590183017006213e-25, 7.62939225751109495e-06, 3.90624883524053534e-03,
      3.99999817212472886e+00, 3.19999497732139844e+01, 2.55997319560658525e+02,
      2.04783273324324227e+03, 3.27253652246982456e+04, 2.59434457346894662e+05,
      1.00621694177949021e+06, 6.17199139831178170e+06, 1.26597313574535716e+07,
      1.62529260000000075e+07, 1.67731180000000000e+07, 1.67769580000000000e+07 },
    /* 15082603 */
    { 9.82298962180288047e-64, 4.21894191745907802e-54, 7.78257418132130597e-35,
      3.34259015874689832e-25, 6.16599052016874108e-06, 3.15698714588672326e-03,
      3.23275437590726122e+00, 2.58620091390967453e+01, 2.06894417561625545e+02,
      1.65504939094220754e+03, 2.64517551029136412e+04, 2.09891694997857179e+05,
      8.16575685588646214e+05, 5.13336480662504770e+06, 1.10033654155580010e+07,
      1.45583150000001676e+07, 1.50785070000000000e+07, 1.50823470000000000e+07 },
    /* 14986273 */
    { 9.69791481108703163e-64, 4.16522269530128191e-54, 7.68347970702294475e-35,
      3.30002940611432092e-25, 6.08747978902901173e-06, 3.11678965155155231e-03,
      3.19159215049388845e+00, 2.55327118282773071e+01, 2.04260070593989951e+02,
      1.63397663226719487e+03, 2.61151435765585957e+04, 2.07231508480752498e+05,
      8.06367654055638355e+05, 5.07635187903902307e+06, 1.09097087114329021e+07,
      1.44619850000002030e+07, 1.49821770000000000e+07, 1.49860170000000000e+07 },
    /* 14776336 */
    { 9.42810913278675722e-64, 4.04934203884380436e-54, 7.46971762574649011e-35,
      3.20821929129359426e-25, 5.91812001988149620e-06, 3.03007744976589765e-03,
      3.10279887462500303e+00, 2.48223666728909436e+01, 1.98577376650443540e+02,
      1.58851938758362576e+03, 2.53890076205234654e+04, 2.01492261805796676e+05,
      7.84335037057878566e+05, 4.95288674782931432e+06, 1.07058149018839840e+07,
      1.42520480000003017e+07, 1.47722400000000000e+07, 1.47760800000000000e+07 },
    /* 14196869 */
    { 8.70314528971027262e-64, 3.73797243916420662e-54, 6.89534209398419660e-35,
      2.96152687883942827e-25, 5.46305284013487504e-06, 2.79708305378238405e-03,
      2.86421266221348869e+00, 2.29136797245160615e+01, 1.83308057120624454e+02,
      1.46637609822502554e+03, 2.34378018895664463e+04, 1.86065371296118683e+05,
      7.25048552277948707e+05, 4.61779125281785242e+06, 1.01446868737243451e+07,
      1.36725810000009108e+07, 1.41927730000000000e+07, 1.41966130000000000e+07 },
    /* 12204240 */
    { 6.43150420527001539e-64, 2.76231002257211870e-54, 5.09556260386307283e-35,
      2.18852747383125011e-25, 4.03712062080382464e-06, 2.06700575761862432e-03,
      2.11661365131384116e+00, 1.69328955058294497e+01, 1.35462286951825348e+02,
      1.08364216400000464e+03, 1.73228893695771148e+04, 1.37669261714004766e+05,
      5.38415595845002681e+05, 3.53292539626187785e+06, 8.23848823565938789e+06,
      1.16799520000407528e+07, 1.22001440000000000e+07, 1.22039840000000000e+07 },
    /* 11017633 */
    { 5.24164589759972754e-64, 2.25126977074033947e-54, 4.15285973017258180e-35,
      1.78363967259666233e-25, 3.29023445600991739e-06, 1.68460004130569592e-03,
      1.72503026241426105e+00, 1.38002320160382475e+01, 1.10401210801834779e+02,
      8.83168387150024387e+02, 1.41193736003445592e+04, 1.12282200585662198e+05,
      4.40082662240044388e+05, 2.94038767245387891e+06, 7.12661430867962260e+06,
      1.04933450003918260e+07, 1.10135370000000000e+07, 1.10173770000000000e+07 },
    /* 9437505 */
    { 3.84596615253128342e-64, 1.65182988466448099e-54, 3.04708831357108469e-35,
      1.30871446548116017e-25, 2.41415208102884383e-06, 1.23604586537905408e-03,
      1.26571085309146980e+00, 1.01256804873721595e+01, 8.10050383096763937e+01,
      6.48014349639423358e+02, 1.03611138831922271e+04, 8.24657129882121953e+04,
      3.24156550320632989e+05, 2.21947546481000213e+06, 5.68524343875118531e+06,
      8.91321700797987171e+06, 9.43340900000000000e+06, 9.43724900000000000e+06 },
    /* 8390657 */
    { 3.04006590453258966e-64, 1.30569836376521308e-54, 2.40858835538382027e-35,
      1.03448082158999336e-25, 1.90828029650285053e-06, 9.77039511733760911e-04,
      1.00048838056196132e+00, 8.00390259075751231e+00, 6.40309356878872933e+01,
      5.12229243608175807e+02, 8.19066683023702899e+03, 6.52277588009487954e+04,
      2.56891072309514391e+05, 1.78809403153571300e+06, 4.76371295024558529e+06,
      7.86636905876981001e+06, 8.38656100000000000e+06, 8.39040100000000000e+06 },
    /* 8388608 */
    { 3.03858131641597245e-64, 1.30506073802432296e-54, 2.40741214349811932e-35,
      1.03397564243176815e-25, 1.90734840543853551e-06, 9.76562383508887020e-04,
      9.99999801317883907e-01, 7.99999396006690677e+00, 6.39996668511303071e+01,
      5.11979106274727883e+02, 8.18666829515939844e+03, 6.51959881527814287e+04,
      2.56766914989349432e+05, 1.78728773698867904e+06, 4.76194118448516913e+06,
      7.86432005899994168e+06, 8.38451200000000000e+06, 8.38835200000000000e+06 },
    /* 8303633 */
    { 2.97733261180485959e-64, 1.27875461970161355e-54, 2.35888592027094511e-35,
      1.01313378825585727e-25, 1.86890197043808392e-06, 9.56877808790931330e-04,
      9.79842799195114300e-01, 7.83873807696676383e+00, 6.27096283547353366e+01,
      5.01659346659709513e+02, 8.02170245095559312e+03, 6.38851939022925071e+04,
      2.51643815255051391e+05, 1.75398342366120382e+06, 4.68858358349586092e+06,
      7.77934506938103493e+06, 8.29953700000000000e+06, 8.30337700000000000e+06 },
    /* 6445069 */
    { 1.79368505410408035e-64, 7.70381864670101568e-55, 1.42110370965965099e-35,
      6.10359395721248029e-26, 1.12591435658525644e-06, 5.76468150537344320e-04,
      5.90303350141551664e-01, 4.72242478267542509e+00, 3.77792690805288558e+01,
      3.02225885259077643e+02, 4.83334738231306892e+03, 3.85317788870130607e+04,
      1.52297025401436375e+05, 1.09355884627841157e+06, 3.15298493161437940e+06,
      5.92078340317591745e+06, 6.44097300000000000e+06, 6.44481300000000000e+06 },
    /* 5471025 */
    { 1.29249369610449219e-64, 5.55121815505495657e-55, 1.02401900603628891e-35,
      4.39812814140828746e-26, 8.11311442279305058e-07, 4.15391458426019348e-04,
      4.25360831402496142e-01, 3.40288541657277221e+00, 2.72230043153551051e+01,
      2.17778977519387723e+02, 3.48307701466327671e+03, 2.77819973005047868e+04,
      1.10006032571945238e+05, 8.02497636826934526e+05, 2.41479032500354247e+06,
      4.94675240411104914e+06, 5.46692900000000000e+06, 5.47076900000000000e+06 },
    /* 5461601 */
    { 1.28804481454968919e-64, 5.53211035427330002e-55, 1.02049423892798245e-35,
      4.38298938195209473e-26, 8.08518834066487105e-07, 4.13961643021164814e-04,
      4.23896700541549154e-01, 3.39117237605436062e+00, 2.71293003988329815e+01,
      2.17029372274540748e+02, 3.47109048311671313e+03, 2.76865308479067826e+04,
      1.09629930206165693e+05, 7.99877169687261223e+05, 2.40792627883238578e+06,
      4.93732868350143358e+06, 5.45750500000000000e+06, 5.46134500000000000e+06 },
    /* 5000000 */
    { 1.07952085348259170e-64, 4.63650676105773906e-55, 8.55284536172561161e-36,
      3.67341911163567920e-26, 6.77626222278107512e-07, 3.46944625790372989e-04,
      3.55271279996754563e-01, 2.84216929754907532e+00, 2.27372940653300759e+01,
      1.81894492427756745e+02, 2.90925341562651647e+03, 2.32109475844556837e+04,
      9.19864480283138982e+04, 6.76244582431662595e+05, 2.07902454915874335e+06,
      4.47574982779582217e+06, 4.99590400000000000e+06, 4.99974400000000000e+06 },
    /* 4720129 */
    { 9.62052468491602810e-65, 4.13198388920750452e-55, 7.62216493209018785e-36,
      3.27369491080454178e-26, 6.03890121950116545e-07, 3.09191742424983634e-04,
      3.16612330098731132e-01, 2.53289784792646122e+00, 2.02631320402621107e+01,
      1.62101808815417854e+02, 2.59273843912307711e+03, 2.06888306707860320e+04,
      8.20335711247183208e+04, 6.05859806423343602e+05, 1.88701706041535083e+06,
      4.19590551232236158e+06, 4.71603300000000000e+06, 4.71987300000000000e+06 },
    /* 4598479 */
    { 9.13102296289999889e-65, 3.92174450046805166e-55, 7.23434171226120578e-36,
      3.10712610622505210e-26, 5.73163600862704501e-07, 2.93459763629244023e-04,
      3.00502784877568652e-01, 2.40402154589327210e+00, 1.92321254470970260e+01,
      1.53854000743080690e+02, 2.46084059619524533e+03, 1.96376437319819379e+04,
      7.78830134114269749e+04, 5.76361321148565039e+05, 1.80542466236221301e+06,
      4.07427236013673665e+06, 4.59438300000000000e+06, 4.59822300000000000e+06 },
    /* 4514873 */
    { 8.80201481185765059e-65, 3.78043657558362023e-55, 6.97367459966819779e-36,
      2.99517043385208020e-26, 5.52511424504064165e-07, 2.82885849334287552e-04,
      2.89675097340006849e-01, 2.31740008485763216e+00, 1.85391562717557470e+01,
      1.48310408165256945e+02, 2.37218721144947949e+03, 1.89310433056085276e+04,
      7.50922424384496408e+04, 5.56476519408195047e+05, 1.75003032936007436e+06,
      3.99068042602826888e+06, 4.51077700000000000e+06, 4.51461700000000000e+06 },
    /* 4216423 */
    { 7.67678466448147999e-65, 3.29715390723822894e-55, 6.08217542984550923e-36,
      2.61227445597212045e-26, 4.81879583396028819e-07, 2.46722346689160995e-04,
      2.52643672927461205e-01, 2.02114881826249349e+00, 1.61691543761076666e+01,
      1.29350920164308604e+02, 2.06897994841936315e+03, 1.65139961617354602e+04,
      6.55409147975342930e+04, 4.88100916845553555e+05, 1.55700132055291533e+06,
      3.69230361198300030e+06, 4.21232700000000000e+06, 4.21616700000000000e+06 },
    /* 4194304 */
    { 7.59645238547202323e-65, 3.26265145612235253e-55, 6.01852964128048457e-36,
      2.58493879793062928e-26, 4.76837044516251121e-07, 2.44140566782865192e-04,
      2.49999930461255154e-01, 1.99999888738057052e+00, 1.59999554953052812e+01,
      1.27997365357353743e+02, 2.04733300825732044e+03, 1.63414126607763610e+04,
      6.48586183619030489e+04, 4.83196861208001501e+05, 1.54299802768340637e+06,
      3.67019187768841069e+06, 4.19020800000000000e+06, 4.19404800000000000e+06 },
    /* 4000000 */
    { 6.90893311684184468e-65, 2.96736417870870697e-55, 5.47382075781328512e-36,
      2.35098811389739960e-26, 4.33680760573953185e-07, 2.22044549405662773e-04,
      2.27373609983355179e-01, 1.81898839734530293e+00, 1.45518762974392430e+01,
      1.16413034003141178e+02, 1.86206657745167763e+03, 1.48642188911844787e+04,
      5.90168968299262124e+04, 4.41096638730170089e+05, 1.42185603096995712e+06,
      3.47596677852119505e+06, 3.99590400000000000e+06, 3.99974400000000000e+06 },
    /* 3981553 */
    { 6.84535550514410596e-65, 2.94005780240874949e-55, 5.42344938429471275e-36,
      2.32935377370571273e-26, 4.29689929206757446e-07, 2.20001243745771501e-04,
      2.25281265106172607e-01, 1.80224964497290951e+00, 1.44179667037433958e+01,
      1.15341784471186955e+02, 1.84493404804906459e+03, 1.47276033582964737e+04,
      5.84764753082058160e+04, 4.37191522377733258e+05, 1.41053273133602901e+06,
      3.45752890244734008e+06, 3.97745700000000000e+06, 3.98129700000000000e+06 },
    /* 3469497 */
    { 5.19785334334943400e-65, 2.23246101190900781e-55, 4.11816369412201186e-36,
      1.76873783858285884e-26, 3.26274542418221493e-07, 1.67052565712777593e-04,
      1.71061821672631342e-01, 1.36849425850745465e+00, 1.09479339161807978e+01,
      8.75821816252248908e+01, 1.40096122943031264e+03, 1.11865973776804603e+04,
      4.44589238065494865e+04, 3.35240937339222815e+05, 1.10925791919939918e+06,
      2.94590981907640956e+06, 3.46540100000000000e+06, 3.46924100000000000e+06 },
    /* 2796417 */
    { 3.37671825984804601e-65, 1.45028944938533875e-55, 2.67531183056124863e-36,
      1.14903768188624562e-26, 2.11960040488029904e-07, 1.08523540727069068e-04,
      1.11128102763280903e-01, 8.89024657235948701e-01, 7.11218670620169569e+00,
      5.68968183484790444e+01, 9.10163898031904523e+02, 7.27026311537105084e+03,
      2.89302976804814243e+04, 2.20626239906953182e+05, 7.55430265292525059e+05,
      2.27465918879699614e+06, 2.79232100000000000e+06, 2.79616100000000000e+06 },
    /* 2396744 */
    { 2.48047143920984062e-65, 1.06535437100683176e-55, 1.96523194297708407e-36,
      8.44060692414111294e-27, 1.55701715756405132e-07, 7.97192784655151597e-05,
      8.16325392969082797e-02, 6.53060210574274436e-01, 5.22447504133784690e+00,
      4.17953751659456785e+01, 6.68609402176202252e+02, 5.34191798810462478e+03,
      2.12726697966660395e+04, 1.63326698532949667e+05, 5.71039962053837837e+05,
      1.87787878976813331e+06, 2.39264800000000000e+06, 2.39648800000000000e+06 },
    /* 2098177 */
    { 1.90096951102133711e-65, 8.16460188052975446e-56, 1.50610321353860109e-36,
      6.46866404654879610e-27, 1.19325790165487525e-07, 6.10948045635459623e-05,
      6.25610786307022049e-02, 5.00488559404961619e-01, 4.00390401824189190e+00,
      3.20309469002191776e+01, 5.12416921058289972e+02, 4.09466699542457309e+03,
      1.63148862712246882e+04, 1.25897567119276093e+05, 4.47225202517700847e+05,
      1.58347287791373348e+06, 2.09408100000000000e+06, 2.09792100000000000e+06 },
    /* 2097152 */
    { 1.89911264358405187e-65, 8.15662669561360700e-56, 1.50463205158771428e-36,
      6.46234545408261769e-27, 1.19209232707357876e-07, 6.10351271449853099e-05,
      6.24999689559159743e-02, 4.99999682108684340e-01, 3.99999300640047295e+00,
      3.19996592233267698e+01, 5.11916432816754536e+02, 4.09066992542314756e+03,
      1.62989912696615120e+04, 1.25777098836656849e+05, 4.46821820522652706e+05,
      1.58246663305044221e+06, 2.09305600000000000e+06, 2.09689600000000000e+06 },
    /* 1271626 */
    { 6.98247791753670586e-66, 2.99895143008623366e-56, 5.53208895202860154e-37,
      2.37601411275257565e-27, 4.38297242534678273e-08, 2.24408188175120292e-05,
      2.29793981925642821e-02, 1.83835170037565776e-01, 1.47068036811238745e+00,
      1.17653794451473974e+01, 1.88228655326640251e+02, 1.50478955000098654e+03,
      6.00493221828217247e+03, 4.69964688955476740e+04, 1.74675738335436967e+05,
      7.93705775574441534e+05, 1.26753000000000000e+06, 1.27137000000000000e+06 },
    /* 1180417 */
    { 6.01674571488324041e-66, 2.58417260737716580e-56, 4.76695707305772932e-37,
      2.04739247302188301e-27, 3.77677249682731562e-08, 1.93370751835450871e-05,
      1.98011647667272750e-02, 1.58409305733226813e-01, 1.26727365222838628e+00,
      1.01381384252463871e+01, 1.62196284074367895e+02, 1.29673859731428774e+03,
      5.17557281139463612e+03, 4.05690452754900689e+04, 1.51559237337625702e+05,
      7.11307437578365323e+05, 1.17632100000000000e+06, 1.18016100000000000e+06 },
    /* 1048576 */
    { 4.74777934504035996e-66, 2.03915570155726458e-56, 3.76157833530725135e-37,
      1.61558559314867667e-27, 2.98022939659853163e-08, 1.52587745104367425e-05,
      1.56249849436188217e-02, 1.24999870856632000e-01, 9.99998410544928107e-01,
      7.99995168077293606e+00, 1.27989461928571330e+02, 1.02333268407003743e+03,
      4.08535025830558106e+03, 3.20958386865916218e+04, 1.20799142289413823e+05,
      5.95242529642230948e+05, 1.04448000000000000e+06, 1.04832000000000000e+06 },
    /* 1000000 */
    { 4.31807995946294477e-66, 1.85460122074063535e-56, 3.42113540777918151e-37,
      1.46936646915992086e-27, 2.71050272070828090e-08, 1.38777739298982540e-05,
      1.42108403697154325e-02, 1.13686715418339940e-01, 9.09493240826389937e-01,
      7.27591504542061607e+00, 1.16406170946493603e+02, 9.30743673031597268e+02,
      3.71605194956770447e+03, 2.92188944778244804e+04, 1.10274089241209091e+05,
      5.53554744840516942e+05, 9.95904000000000000e+05, 9.99744000000000000e+05 },
    /* 819841 */
    { 2.90235045358949550e-66, 1.24655002796976490e-56, 2.29947893410337365e-37,
      9.87618681981492889e-28, 1.82183490689266710e-08, 9.32779472321984348e-06,
      9.55166172246109217e-03, 7.64132896251342869e-02, 6.11306051109687054e-01,
      4.89043139187867482e+00, 7.82422349713714453e+01, 6.25659192034058037e+02,
      2.49882041253832767e+03, 1.97089496950203138e+04, 7.51500479695295217e+04,
      4.05315292462697893e+05, 8.15745000000000000e+05, 8.19585000000000000e+05 },
    /* 652545 */
    { 1.83870213969147930e-66, 7.89716555706012712e-57, 1.45676991938802090e-37,
      6.25677916156810610e-28, 1.15417203919164522e-08, 5.90936084062561679e-06,
      6.05118546342795372e-03, 4.84094816125079305e-02, 3.87275718825497939e-01,
      3.09819716985184357e+00, 4.95688012285943671e+01, 3.96409870457700265e+02,
      1.58371435214666167e+03, 1.25273157680588301e+04, 4.82278663969549234e+04,
      2.79276527717245917e+05, 6.48449000000000000e+05, 6.52289000000000000e+05 },
    /* 524801 */
    { 1.18926762015466819e-66, 5.10786553475605035e-57, 9.42234882825664415e-38,
      4.04686800688662073e-28, 7.46515384231198445e-09, 3.82215876724521482e-06,
      3.91389055821865072e-03, 3.13111233760198990e-02, 2.50488917265499822e-01,
      2.00390687460218908e+00, 3.20612857504726705e+01, 2.56417175606829119e+02,
      1.02466699434609745e+03, 8.12310498202530835e+03, 3.15045400686032553e+04,
      1.93198962659155397e+05, 5.20705000000000000e+05, 5.24545000000000000e+05 },
    /* 401857 */
    { 6.97321585851295025e-67, 2.99497340602616845e-57, 5.52475079285309336e-38,
      2.37286239738541065e-28, 4.37715853666972486e-09, 2.24110517076658296e-06,
      2.29489168613654978e-03, 1.83591329998224681e-02, 1.46873032685309740e-01,
      1.17498225743608220e+00, 1.87991664504370917e+01, 1.50360504164546711e+02,
      6.00992138052254290e+02, 4.77454013471333201e+03, 1.86505860938960723e+04,
      1.21176669942356806e+05, 3.97761000000000000e+05, 4.01601000000000000e+05 },
    /* 264097 */
    { 3.01173257048041585e-67, 1.29352928945114011e-57, 2.38614037543525460e-38,
      1.02483948761595803e-28, 1.89049517446831162e-09, 9.67933529325415291e-07,
      9.91163931551744364e-04, 7.92931131353941630e-03, 6.34344816203459005e-02,
      5.07475284133261262e-01, 8.11944852670449713e+00, 6.49462697901977464e+01,
      2.59657344516898661e+02, 2.06775748649864772e+03, 8.14269081216647010e+03,
      5.66232434728111548e+04, 2.60001000000000000e+05, 2.63841000000000000e+05 },
    /* 204800 */
    { 1.81112697232874206e-67, 7.77873111505544409e-58, 1.43492262097629106e-38,
      6.16294572938377368e-29, 1.13686282610103304e-09, 5.82073766962628089e-07,
      5.96043536214395245e-04, 4.76834822495310166e-03, 3.81467816548533359e-02,
      3.05173987973646754e-01, 4.88271104998955341e+00, 3.90573427578099199e+01,
      1.56169795671348624e+02, 1.24492319174046884e+03, 4.91958032892884057e+03,
      3.52628737812490363e+04, 2.00704000000000000e+05, 2.04544000000000000e+05 },
    /* 200000 */
    { 1.72722507485033383e-67, 7.41837520931333590e-58, 1.36844868928954633e-38,
      5.87744236675266698e-29, 1.08419675147463808e-09, 5.55108736753989574e-07,
      5.68431345360095224e-04, 4.54745070256641366e-03, 3.63796017604134173e-02,
      2.91036567035938998e-01, 4.65651731179381212e+00, 3.72480912910018702e+01,
      1.48936880685972909e+02, 1.18736413772828405e+03, 4.69345257857060551e+03,
      3.37256310720094916e+04, 1.95904000000000000e+05, 1.99744000000000000e+05 },
    /* 102774 */
    { 4.56093001325520124e-68, 1.95890452462759358e-58, 3.61354104306368883e-39,
      1.55200406027122712e-29, 2.86294217011813689e-10, 1.46582639109909510e-07,
      1.50100622302544283e-04, 1.20080497023619128e-03, 9.60643923810312883e-03,
      7.68514803825075948e-02, 1.22961449148191515e+00, 9.83636673154418517e+00,
      3.93379364327392551e+01, 3.14142047803753769e+02, 1.24891387725365462e+03,
      9.44593016329059901e+03, 9.86780000000517612e+04, 1.02518000000000000e+05 },
    /* 100000 */
    { 4.31804109670444684e-68, 1.85458452931295726e-58, 3.42110461752972125e-39,
      1.46935324484847411e-29, 2.71047832615944429e-10, 1.38776490299235408e-07,
      1.42107125931920287e-04, 1.13685699991618186e-03, 9.09485551682193138e-03,
      7.27588132541049926e-02, 1.16413254204269756e+00, 9.31255441700961661e+00,
      3.72432805975374706e+01, 2.97429023684080164e+02, 1.18266355295424069e+03,
      8.95817783366734693e+03, 9.59040000001018925e+04, 9.97440000000000000e+04 },
    /* 77163 */
    { 2.57100957639565332e-68, 1.10424020483221446e-58, 2.03696364544404734e-39,
      8.74869224032312274e-30, 1.61384886736889072e-10, 8.26290620092283361e-08,
      8.46121594356573939e-05, 6.76897272021500683e-04, 5.41517795449147563e-03,
      4.33214094483818091e-02, 6.93138659749164665e-01, 5.54487683849644686e+00,
      2.21763200560975164e+01, 1.77172840383531820e+02, 7.05445676326827083e+02,
      5.40962011023344166e+03, 7.30670000269061129e+04, 7.69070000000000000e+04 },
    /* 50643 */
    { 1.10744301397987420e-68, 4.75643152722723048e-59, 8.77406750868841857e-40,
      3.76843330027129536e-30, 6.95153246489491803e-11, 3.55918462202453374e-08,
      3.64460505120626550e-05, 2.91568403117305078e-04, 2.33254716226988625e-03,
      1.86603732873723421e-02, 2.98564872499666734e-01, 2.38845326899687205e+00,
      9.55291197889362387e+00, 7.63560630702938568e+01, 3.04504893070908849e+02,
      2.36897008846858444e+03, 4.65470174614963616e+04, 5.03870000000000000e+04 },
    /* 6 */
    { 1.29542528326416669e-76, 5.56380922603113208e-67, 1.02634164867540313e-47,
      4.40810381558357815e-38, 8.13151629364128326e-19, 4.16333634234433703e-16,
      4.26325641456043956e-13, 3.41060513164744692e-12, 2.72848410531216727e-11,
      2.18278728421267612e-10, 3.49245965372384226e-09, 2.79396771690754164e-08,
      1.11758707843634397e-07, 8.94069600576588975e-07, 3.57627754965526357e-06,
      2.86101567327154416e-05, 3.66091750036190520e-03, 5.82894668923472636e-02 },
};

void printdouble( const int width, const double value )
{
    if (width < 10)
        printf("%.*s|", width - 1, "----------");
    else if (value == 0.0)
        printf (" %*.3f |", width - 2, value);
    else if (value < 1.0e-100)
        printf (" %.*e |", width - 9, value);
    else if (value < 1.0e-6)
        printf (" %.*e  |", width - 9, value);
    else if (value < 1.0)
        printf ("  %*.*f |", width - 3, width - 5, value);
    else if (value < 1.0e6)
        printf (" %*.3f |", width - 2, value);
    else
        printf (" %*.1f   |", width - 4, value);
}

void ReportCollisionEstimates( void )
{
    const int keys[] = {
      149633745, 86536545, 75498113, 56050289, 49925029, 44251425,
      43691201, 33558529, 33554432, 26977161, 22370049, 18877441,
      18616785, 17676661, 16777216, 16777214, 15082603, 14986273,
      14776336, 14196869, 12204240, 11017633, 9437505, 8390657,
      8388608, 8303633, 6445069, 5471025, 5461601, 5000000,
      4720129, 4598479, 4514873, 4216423, 4194304, 4000000,
      3981553, 3469497, 2796417, 2396744, 2098177, 2097152,
      1271626, 1180417, 1048576, 1000000, 819841, 652545,
      524801, 401857, 264097, 204800, 200000, 102774,
      100000, 77163, 50643, 6
    };
    const int bits[] = { 256, 224, 160, 128, 64, 55, 45, 42, 39, 36, 32, 29, 27, 24, 22, 19, 12, 8 };
    printf ("EstimateNbCollisions:\n");
    printf ("  # keys   : bits|    True answer     |   A: _fwojcik()    |   B: _previmpl()   |   C: _Demerphq()   |    Error A   |    Error B   |    Error C   |\n");
    printf ("---------------------------------------------------------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) {
      const int key = keys[i];
      for (int j = 0; j < sizeof(bits)/sizeof(bits[0]); j++) {
        const int bit = bits[j];
        printf (" %9d : %3d |", key, bit);
        printdouble(20, realcoll[i][j]);
        for (int k = 0; k < COLLISION_ESTIMATORS; k++) {
            printdouble(20, EstimateNbCollisionsCand(key, bit, k));
        }
        for (int k = 0; k < COLLISION_ESTIMATORS; k++) {
            double delta = EstimateNbCollisionsCand(key, bit, k) - realcoll[i][j];
            double deltapct = delta/realcoll[i][j]*100.0;
            if (deltapct > 9999.999)
                deltapct = 9999.999;
            printf(" %+11.5f%% |", deltapct);
        }
        printf("\n");
      }
    }
}

//-----------------------------------------------------------------------------

#ifdef _WIN32
static char* strndup(char const *s, size_t n)
{
  size_t const len = strnlen(s, n);
  char *p = (char*) malloc(len + 1);
  if (p == NULL) return NULL;
  memcpy(p, s, len);
  p[len] = '\0';
  return p;
}
#endif

void usage( void )
{
    printf("Usage: SMHasher [--list][--listnames][--tests] [--verbose][--extra]\n"
           "       [--test=Speed,...] hash\n");
}

int main ( int argc, const char ** argv )
{
  setbuf(stdout, NULL); // Unbuffer stdout always
  setbuf(stderr, NULL); // Unbuffer stderr always

#if defined(__x86_64__) || defined(_M_X64) || defined(_X86_64_)
  const char * defaulthash = "xxh3";
#else
  const char * defaulthash = "wyhash";
#endif
  const char * hashToTest = defaulthash;

  if (argc < 2) {
    printf("No test hash given on command line, testing %s.\n", hashToTest);
    usage();
  }

  for (int argnb = 1; argnb < argc; argnb++) {
    const char* const arg = argv[argnb];
    if (strncmp(arg,"--", 2) == 0) {
      // This is a command
      if (strcmp(arg,"--help") == 0) {
        usage();
        exit(0);
      }
      if (strcmp(arg,"--list") == 0) {
        for(size_t i = 0; i < sizeof(g_hashes) / sizeof(HashInfo); i++) {
          printf("%-16s\t\"%s\" %s\n", g_hashes[i].name, g_hashes[i].desc, quality_str[g_hashes[i].quality]);
        }
        exit(0);
      }
      if (strcmp(arg,"--listnames") == 0) {
        for(size_t i = 0; i < sizeof(g_hashes) / sizeof(HashInfo); i++) {
          printf("%s\n", g_hashes[i].name);
        }
        exit(0);
      }
      if (strcmp(arg,"--tests") == 0) {
        printf("Valid tests:\n");
        for(size_t i = 0; i < sizeof(g_testopts) / sizeof(TestOpts); i++) {
          printf("  %s\n", g_testopts[i].name);
        }
        exit(0);
      }
      if (strcmp(arg,"--verbose") == 0) {
        g_drawDiagram = true;
        continue;
      }
      if (strcmp(arg,"--extra") == 0) {
        g_testExtra = true;
        continue;
      }
      if (strcmp(arg,"--EstimateNbCollisions") == 0) {
        ReportCollisionEstimates();
        exit(0);
      }
      /* default: --test=All. comma seperated list of options */
      if (strncmp(arg,"--test=", 6) == 0) {
        char *opt = (char *)&arg[7];
        char *rest = opt;
        char *p;
        bool found = false;
        bool need_opt_free = false;
        g_testAll = false;
        do {
          if ((p = strchr(rest, ','))) {
            opt = strndup(rest, p-rest);
            need_opt_free = true;
            rest = p+1;
          } else {
            need_opt_free = false;
            opt = rest;
          }
          for (size_t i = 0; i < sizeof(g_testopts) / sizeof(TestOpts); i++) {
            if (strcmp(opt, g_testopts[i].name) == 0) {
              g_testopts[i].var = true; found = true; break;
            }
          }
          if (!found) {
            printf("Invalid option: --test=%s\n", opt);
            printf("Valid tests: --test=%s", g_testopts[0].name);
            for(size_t i = 1; i < sizeof(g_testopts) / sizeof(TestOpts); i++) {
              printf(",%s", g_testopts[i].name);
            }
            printf(" \n");
            if (need_opt_free)
              free(opt);
            exit(1);
          }
          if (need_opt_free)
            free(opt);
        } while (p);
        continue;
      }
      // invalid command
      printf("Invalid command \n");
      usage();
      exit(1);
    }
    // Not a command ? => interpreted as hash name
    hashToTest = arg;
  }

  // Code runs on the 3rd CPU by default? only for speed tests
  //SetAffinity((1 << 2));
  //SelfTest();

  clock_t timeBegin = clock();

  testHash(hashToTest);

  clock_t timeEnd = clock();

  printf("\n");
  fflush(NULL);
  if (g_testAll) {
    printf("Input vcode 0x%08x, Output vcode 0x%08x, Result vcode 0x%08x\n", g_inputVCode, g_outputVCode, g_resultVCode);
    printf("Verification value is 0x%08x - Testing took %f seconds\n", g_verify, double(timeEnd-timeBegin)/double(CLOCKS_PER_SEC));
    printf("-------------------------------------------------------------------------------\n");
  } else {
    fprintf(stderr, "Input vcode 0x%08x, Output vcode 0x%08x, Result vcode 0x%08x\n", g_inputVCode, g_outputVCode, g_resultVCode);
    fprintf(stderr, "Verification value is 0x%08x - Testing took %f seconds\n", g_verify, double(timeEnd-timeBegin)/double(CLOCKS_PER_SEC));
  }
    fflush(NULL);
  return 0;
}
