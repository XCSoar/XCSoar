// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ByteOrder.hxx"
#include <limits>
#include <cstring>
#include <utility>

#ifdef __cpp_lib_bit_cast
#include <bit>
#endif

/**
 * Classes for floats rely on assumption that float type is 32bit and that the
 * platform uses IEEE754 floating point representation (aso known as IEC559)
 */
static_assert(std::numeric_limits<float>::is_iec559);
static_assert(sizeof(float) == 4);

/**
 * Float represented by IEEE754 standard. With bytes swapped (little-endian)
 */
class PackedFloatLE
{
  uint8_t a, b, c, d;

public:
  PackedFloatLE() = default;

  constexpr PackedFloatLE(float f)
  {
    uint32_t bytes;
#if defined(__cpp_lib_bit_cast)
    bytes = std::bit_cast<uint32_t>(f);
#elif defined(__has_builtin) && __has_builtin(__builtin_bit_cast)
    bytes = __builtin_bit_cast(uint32_t, f);
#else
#error "No constexpr bit-cast available."
#endif
    a = uint8_t(bytes);
    b = uint8_t(bytes >> 8);
    c = uint8_t(bytes >> 16);
    d = uint8_t(bytes >> 24);
  }
};

static_assert(sizeof(PackedFloatLE) == sizeof(float), "Wrong size");
static_assert(alignof(PackedFloatLE) == 1, "Wrong alignment");

/**
 * Float represented by IEEE754 standard. Bytes are not swapped (big-endian)
 */
class PackedFloatBE
{
  uint8_t a, b, c, d;

public:
  PackedFloatBE() = default;

  constexpr PackedFloatBE(float f)
  {
    uint32_t bytes;
#if defined(__cpp_lib_bit_cast)
    bytes = std::bit_cast<uint32_t>(f);
#elif defined(__has_builtin) && __has_builtin(__builtin_bit_cast)
    bytes = __builtin_bit_cast(uint32_t, f);
#else
#error "No constexpr bit-cast available."
#endif
    d = uint8_t(bytes);
    c = uint8_t(bytes >> 8);
    b = uint8_t(bytes >> 16);
    a = uint8_t(bytes >> 24);
  }
};

static_assert(sizeof(PackedFloatBE) == sizeof(float), "Wrong size");
static_assert(alignof(PackedFloatBE) == 1, "Wrong alignment");
