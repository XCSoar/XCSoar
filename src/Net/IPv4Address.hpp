/*
 * Copyright (C) 2012-2015 Max Kellermann <max@duempel.org>
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

#ifndef IPV4_ADDRESS_HPP
#define IPV4_ADDRESS_HPP

#include "SocketAddress.hpp"
#include "OS/ByteOrder.hpp"
#include "Compiler.h"

#include <stdint.h>

#ifdef HAVE_POSIX
#include <netinet/in.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

/**
 * An OO wrapper for struct sockaddr_in.
 */
class IPv4Address {
  struct sockaddr_in address;

#ifdef WIN32
  static constexpr struct in_addr ConstructInAddr(uint8_t a, uint8_t b,
                                                  uint8_t c, uint8_t d) {
    return {{{ a, b, c, d }}};
  }

  static constexpr struct in_addr ConstructInAddr(uint32_t x) {
    return ConstructInAddr(x >> 24, x >> 16, x >> 8, x);
  }
#else

#ifdef __BIONIC__
  typedef uint32_t in_addr_t;
#endif

  static constexpr in_addr_t ConstructInAddrT(uint8_t a, uint8_t b,
                                              uint8_t c, uint8_t d) {
    return ToBE32((a << 24) | (b << 16) | (c << 8) | d);
  }

  static constexpr struct in_addr ConstructInAddr(uint32_t x) {
    return { ToBE32(x) };
  }

  static constexpr struct in_addr ConstructInAddr(uint8_t a, uint8_t b,
                                                  uint8_t c, uint8_t d) {
    return { ConstructInAddrT(a, b, c, d) };
  }
#endif

public:
  IPv4Address() = default;

  constexpr IPv4Address(uint8_t a, uint8_t b, uint8_t c,
                        uint8_t d, uint16_t port)
#if defined(__APPLE__)
    :address{sizeof(struct sockaddr_in), AF_INET, ToBE16(port),
      ConstructInAddr(a, b, c, d), {}} {}
#else
    :address{AF_INET, ToBE16(port), ConstructInAddr(a, b, c, d), {}} {}
#endif

  constexpr IPv4Address(uint16_t port)
#if defined(__APPLE__)
    :address{sizeof(struct sockaddr_in), AF_INET, ToBE16(port),
      ConstructInAddr(INADDR_ANY), {}} {}
#else
    :address{AF_INET, ToBE16(port), ConstructInAddr(INADDR_ANY), {}} {}
#endif

  operator SocketAddress() const {
    return SocketAddress(reinterpret_cast<const struct sockaddr *>(&address),
                         sizeof(address));
  }

  SocketAddress::size_type GetSize() {
    return sizeof(address);
  }

  constexpr bool IsDefined() const {
    return address.sin_family != AF_UNSPEC;
  }

#if defined(__GLIBC__) || defined(__APPLE__)
  /**
   * Returns a StaticSocketAddress for the specified device. Caller
   * should check for validity of returned StaticSocketAddress.
   *
   * @param device is the device name f.i. "eth0"
   * @return StaticSocketAddress, use IsDefined() to check valid result
   */
  gcc_pure
  static IPv4Address GetDeviceAddress(const char *device);

  /**
   * Converts StaticSocketAddress to human readable string
   *
   * @param buffer is the result buffer
   * @param buffer_size is the buffer size
   * @return IP address on success, else NULL
   */
  gcc_pure
  const char *ToString(char *buffer, size_t buffer_size) const;
#endif
};

#endif
