/*
 * Copyright (C) 2011-2015 Max Kellermann <max.kellermann@gmail.com>,
 *                    Tobias Bieniek <Tobias.Bieniek@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XCSOAR_BYTE_ORDER_HPP
#define XCSOAR_BYTE_ORDER_HPP

#include "Compiler.h"

#include <stdint.h>

#if defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__)
/* well-known little-endian */
#  define IS_LITTLE_ENDIAN true
#  define IS_BIG_ENDIAN false
#elif defined(__MIPSEB__)
/* well-known big-endian */
#  define IS_LITTLE_ENDIAN false
#  define IS_BIG_ENDIAN true
#elif defined(__APPLE__)
/* compile-time check for MacOS */
#  include <machine/endian.h>
#  if BYTE_ORDER == LITTLE_ENDIAN
#    define IS_LITTLE_ENDIAN true
#    define IS_BIG_ENDIAN false
#  else
#    define IS_LITTLE_ENDIAN false
#    define IS_BIG_ENDIAN true
#  endif
#else
/* generic compile-time check */
#  include <endian.h>
#  if __BYTE_ORDER == __LITTLE_ENDIAN
#    define IS_LITTLE_ENDIAN true
#    define IS_BIG_ENDIAN false
#  else
#    define IS_LITTLE_ENDIAN false
#    define IS_BIG_ENDIAN true
#  endif
#endif

/* x86 always allows unaligned access */
#if defined(__i386__) || defined(__x86_64__) || \
  /* ARM has it from ARMv6 on */ \
  defined(__ARM_ARCH_6__) || \
  defined(__ARM_ARCH_7__) || \
  defined(__ARM_ARCH_7A__) || \
  /* _M_ARM is the Microsoft way of checking the ARM generation \
     (supported by mingw32ce) */ \
  (defined(_M_ARM) && _M_ARM >= 6)
#ifndef FORCE_ALIGNED_READ_WRITE
#define CAN_READ_WRITE_UNALIGNED
#endif
#endif

static inline constexpr bool
IsLittleEndian()
{
  return IS_LITTLE_ENDIAN;
}

static inline constexpr bool
IsBigEndian()
{
  return IS_BIG_ENDIAN;
}

static inline constexpr uint16_t
GenericByteSwap16(uint16_t value)
{
  return (value >> 8) | (value << 8);
}

static inline constexpr uint32_t
GenericByteSwap32(uint32_t value)
{
  return (value >> 24) | ((value >> 8) & 0x0000ff00) |
    ((value << 8) & 0x00ff0000) | (value << 24);
}

static inline constexpr uint64_t
GenericByteSwap64(uint64_t value)
{
  return uint64_t(GenericByteSwap32(uint32_t(value >> 32)))
    | (uint64_t(GenericByteSwap32(value)) << 32);
}

static inline constexpr uint16_t
ByteSwap16(uint16_t value)
{
#if CLANG_OR_GCC_VERSION(4,8)
  return __builtin_bswap16(value);
#else
  return GenericByteSwap16(value);
#endif
}

static inline constexpr uint32_t
ByteSwap32(uint32_t value)
{
#if CLANG_OR_GCC_VERSION(4,3)
  return __builtin_bswap32(value);
#else
  return GenericByteSwap32(value);
#endif
}

static inline constexpr uint64_t
ByteSwap64(uint64_t value)
{
#if CLANG_OR_GCC_VERSION(4,3)
  return __builtin_bswap64(value);
#else
  return GenericByteSwap64(value);
#endif
}

/**
 * Converts a 16bit value from big endian to the system's byte order
 */
static inline constexpr uint16_t
FromBE16(uint16_t value)
{
  return IsBigEndian() ? value : ByteSwap16(value);
}

/**
 * Converts a 32bit value from big endian to the system's byte order
 */
static inline constexpr uint32_t
FromBE32(uint32_t value)
{
  return IsBigEndian() ? value : ByteSwap32(value);
}

/**
 * Converts a 64bit value from big endian to the system's byte order
 */
static inline constexpr uint64_t
FromBE64(uint64_t value)
{
  return IsBigEndian() ? value : ByteSwap64(value);
}

/**
 * Converts a 16bit value from little endian to the system's byte order
 */
static inline constexpr uint16_t
FromLE16(uint16_t value)
{
  return IsLittleEndian() ? value : ByteSwap16(value);
}

/**
 * Converts a 32bit value from little endian to the system's byte order
 */
static inline constexpr uint32_t
FromLE32(uint32_t value)
{
  return IsLittleEndian() ? value : ByteSwap32(value);
}

/**
 * Converts a 64bit value from little endian to the system's byte order
 */
static inline constexpr uint64_t
FromLE64(uint64_t value)
{
  return IsLittleEndian() ? value : ByteSwap64(value);
}

/**
 * Converts a 16bit value from the system's byte order to big endian
 */
static inline constexpr uint16_t
ToBE16(uint16_t value)
{
  return IsBigEndian() ? value : ByteSwap16(value);
}

/**
 * Converts a 32bit value from the system's byte order to big endian
 */
static inline constexpr uint32_t
ToBE32(uint32_t value)
{
  return IsBigEndian() ? value : ByteSwap32(value);
}

/**
 * Converts a 64bit value from the system's byte order to big endian
 */
static inline constexpr uint64_t
ToBE64(uint64_t value)
{
  return IsBigEndian() ? value : ByteSwap64(value);
}

/**
 * Converts a 16bit value from the system's byte order to little endian
 */
static inline constexpr uint16_t
ToLE16(uint16_t value)
{
  return IsLittleEndian() ? value : ByteSwap16(value);
}

/**
 * Converts a 32bit value from the system's byte order to little endian
 */
static inline constexpr uint32_t
ToLE32(uint32_t value)
{
  return IsLittleEndian() ? value : ByteSwap32(value);
}

/**
 * Converts a 64bit value from the system's byte order to little endian
 */
static inline constexpr uint64_t
ToLE64(uint64_t value)
{
  return IsLittleEndian() ? value : ByteSwap64(value);
}

gcc_pure
static inline uint16_t
ReadUnalignedLE16(const uint16_t *p)
{
#ifdef CAN_READ_WRITE_UNALIGNED
  return FromLE16(*p);
#else
  const uint8_t *c = (const uint8_t *)p;
  return c[0] | (c[1] << 8);
#endif
}

gcc_pure
static inline uint16_t
ReadUnalignedBE16(const uint16_t *p)
{
#ifdef CAN_READ_WRITE_UNALIGNED
  return FromBE16(*p);
#else
  const uint8_t *c = (const uint8_t *)p;
  return c[1] | (c[0] << 8);
#endif
}

static inline void
WriteUnalignedLE16(uint16_t *p, uint16_t value)
{
#ifdef CAN_READ_WRITE_UNALIGNED
  *p = ToLE16(value);
#else
  uint8_t *c = (uint8_t *)p;
  c[0] = value;
  c[1] = value >> 8;
#endif
}

static inline void
WriteUnalignedBE16(uint16_t *p, uint16_t value)
{
#ifdef CAN_READ_WRITE_UNALIGNED
  *p = ToBE16(value);
#else
  uint8_t *c = (uint8_t *)p;
  c[0] = value >> 8;
  c[1] = value;
#endif
}

gcc_pure
static inline uint32_t
ReadUnalignedLE32(const uint32_t *p)
{
#ifdef CAN_READ_WRITE_UNALIGNED
  return FromLE32(*p);
#else
  const uint8_t *c = (const uint8_t *)p;
  return c[0] | (c[1] << 8) | (c[2] << 16) | (c[3] << 24);
#endif
}

gcc_pure
static inline uint32_t
ReadUnalignedBE32(const uint32_t *p)
{
#ifdef CAN_READ_WRITE_UNALIGNED
  return FromBE32(*p);
#else
  const uint8_t *c = (const uint8_t *)p;
  return c[3] | (c[2] << 8) | (c[1] << 16) | (c[0] << 24);
#endif
}

/**
 * A packed big-endian 16 bit integer.
 */
class PackedBE16 {
	uint8_t hi, lo;

public:
	constexpr operator uint16_t() const {
		return (hi << 8) | lo;
	}

	PackedBE16 &operator=(uint16_t new_value) {
		lo = uint8_t(new_value);
		hi = uint8_t(new_value >> 8);
		return *this;
	}
};

static_assert(sizeof(PackedBE16) == sizeof(uint16_t), "Wrong size");
static_assert(alignof(PackedBE16) == 1, "Wrong alignment");

/**
 * A packed little-endian 16 bit integer.
 */
class PackedLE16 {
	uint8_t lo, hi;

public:
	constexpr operator uint16_t() const {
		return (hi << 8) | lo;
	}

	PackedLE16 &operator=(uint16_t new_value) {
		lo = uint8_t(new_value);
		hi = uint8_t(new_value >> 8);
		return *this;
	}
};

static_assert(sizeof(PackedLE16) == sizeof(uint16_t), "Wrong size");
static_assert(alignof(PackedLE16) == 1, "Wrong alignment");

#endif
