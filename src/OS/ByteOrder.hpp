/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
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

#endif
