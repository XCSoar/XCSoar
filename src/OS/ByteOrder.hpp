/*
 * Copyright (C) 2011 Max Kellermann <max@duempel.org>,
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

#ifdef __linux__
#include <features.h>

#if defined(ANDROID) || (defined(__GLIBC__) && ((__GLIBC__ >= 2 && __GLIBC_MINOR__ >= 9) || __GLIBC__ >= 3))
/* the byte swap macros were added in glibc 2.9 */
#define HAVE_BYTESWAP_H
#include <byteswap.h>
#include <endian.h>
#endif
#endif /* !__linux__ */

#if defined(__i386__) || defined(__x86_64__)
#ifndef FORCE_ALIGNED_READ_WRITE
#define CAN_READ_WRITE_UNALIGNED
#endif
#endif

gcc_const
static inline uint16_t
ByteSwap16(uint16_t value)
{
#ifdef HAVE_BYTESWAP_H
  return bswap_16(value);
#else
  return (value >> 8) | (value << 8);
#endif
}

gcc_const
static inline uint32_t
ByteSwap32(uint32_t value)
{
#ifdef HAVE_BYTESWAP_H
  return bswap_32(value);
#else
  return (value >> 24) | ((value >> 8) & 0x0000ff00) |
    ((value << 8) & 0x00ff0000) | (value << 24);
#endif
}

/**
 * Converts a 16bit value from big endian to the system's byte order
 */
gcc_const
static inline uint16_t
FromBE16(uint16_t value)
{
#ifdef HAVE_BYTESWAP_H
#ifdef __BIONIC__
  return betoh16(value);
#else
  return be16toh(value);
#endif
#elif defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__)
  /* generic little-endian */
  return ByteSwap16(value);
#else
  /* generic big-endian */
  return value;
#endif
}

/**
 * Converts a 32bit value from big endian to the system's byte order
 */
gcc_const
static inline uint32_t
FromBE32(uint32_t value)
{
#ifdef HAVE_BYTESWAP_H
#ifdef __BIONIC__
  return betoh32(value);
#else
  return be32toh(value);
#endif
#elif defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__)
  /* generic little-endian */
  return ByteSwap32(value);
#else
  /* generic big-endian */
  return value;
#endif
}

/**
 * Converts a 16bit value from little endian to the system's byte order
 */
gcc_const
static inline uint16_t
FromLE16(uint16_t value)
{
#ifdef HAVE_BYTESWAP_H
#ifdef __BIONIC__
  return letoh16(value);
#else
  return le16toh(value);
#endif
#elif defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__)
  /* generic little-endian */
  return value;
#else
  /* generic big-endian */
  return ByteSwap16(value);
#endif
}

/**
 * Converts a 32bit value from little endian to the system's byte order
 */
gcc_const
static inline uint32_t
FromLE32(uint32_t value)
{
#ifdef HAVE_BYTESWAP_H
#ifdef __BIONIC__
  return letoh32(value);
#else
  return le32toh(value);
#endif
#elif defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__)
  /* generic little-endian */
  return value;
#else
  /* generic big-endian */
  return ByteSwap32(value);
#endif
}

/**
 * Converts a 16bit value from the system's byte order to big endian
 */
gcc_const
static inline uint16_t
ToBE16(uint16_t value)
{
#ifdef HAVE_BYTESWAP_H
  return htobe16(value);
#elif defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__)
  /* generic little-endian */
  return ByteSwap16(value);
#else
  /* generic big-endian */
  return value;
#endif
}

/**
 * Converts a 32bit value from the system's byte order to big endian
 */
gcc_const
static inline uint32_t
ToBE32(uint32_t value)
{
#ifdef HAVE_BYTESWAP_H
  return htobe32(value);
#elif defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__)
  /* generic little-endian */
  return ByteSwap32(value);
#else
  /* generic big-endian */
  return value;
#endif
}

/**
 * Converts a 16bit value from the system's byte order to little endian
 */
gcc_const
static inline uint16_t
ToLE16(uint16_t value)
{
#ifdef HAVE_BYTESWAP_H
  return htole16(value);
#elif defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__)
  /* generic little-endian */
  return value;
#else
  /* generic big-endian */
  return ByteSwap16(value);
#endif
}

/**
 * Converts a 32bit value from the system's byte order to little endian
 */
gcc_const
static inline uint32_t
ToLE32(uint32_t value)
{
#ifdef HAVE_BYTESWAP_H
  return htole32(value);
#elif defined(__i386__) || defined(__x86_64__) || defined(__ARMEL__)
  /* generic little-endian */
  return value;
#else
  /* generic big-endian */
  return ByteSwap32(value);
#endif
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

#endif
