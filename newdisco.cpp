// also known as BEBB4185
// Copyright 2020 Cris Stringfellow
// https://github.com/dosyago/discohash
#include <cstdio>
#include <inttypes.h>
#include <cstring>
#include <iostream>
#include "discohash.h"

alignas(uint64_t) uint8_t disco_buf[STATE] = {0};
constexpr uint64_t P = 0xFFFFFFFFFFFFFFFF - 58;
constexpr uint64_t Q = 13166748625691186689U;
alignas(uint64_t) uint8_t *ds8 = (uint8_t *)disco_buf;
uint64_t *ds = (uint64_t *)disco_buf;

  static inline void turn( const uint64_t * m64, const uint8_t * m8, int len )
  {
    X = (X * (m64  + Y) * P) % Q
    Y = (Y * (m64 + X) * Q) % P
  }

  void NewDisco_64 ( const void * key, int len, unsigned seed, void * out )
  {

  }
