/*
 * Copyright (C) 2012 Max Kellermann <max@duempel.org>
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

#ifndef XCSOAR_SOCKET_ADDRESS_HPP
#define XCSOAR_SOCKET_ADDRESS_HPP

#include "Compiler.h"

#include <assert.h>
#include <stdint.h>

#ifdef HAVE_POSIX
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif

/**
 * An OO wrapper for a UNIX socket address.
 */
class SocketAddress {
  size_t length;
  struct sockaddr_storage address;

public:
  SocketAddress() = default;

#if defined(HAVE_POSIX) && !defined(__BIONIC__)
  /**
   * Make this a "local" address (UNIX domain socket).
   */
  void SetLocal(const char *path);
#endif

  /**
   * Creates a #SocketAddress with the specified IPv4 address and
   * port.
   *
   * @parm ip the IPv4 address in host byte order
   */
  gcc_const
  static SocketAddress MakeIPv4Port(uint32_t ip, unsigned port);

  gcc_const
  static SocketAddress MakeIPv4Port(uint8_t a, uint8_t b, uint8_t c,
                                    uint8_t d, unsigned port) {
    uint32_t ip = (a << 24) | (b << 16) | (c << 8) | d;
    return MakeIPv4Port(ip, port);
  }

  /**
   * Creates a #SocketAddress with the IPv4 a wildcard address and the
   * specified port.
   */
  gcc_const
  static SocketAddress MakePort4(unsigned port);

  operator struct sockaddr *() {
    return reinterpret_cast<struct sockaddr *>(&address);
  }

  operator const struct sockaddr *() const {
    return reinterpret_cast<const struct sockaddr *>(&address);
  }

  constexpr size_t GetCapacity() const {
    return sizeof(address);
  }

  size_t GetLength() const {
    return length;
  }

  void SetLength(size_t _length) {
    assert(_length > 0);
    assert(_length <= sizeof(address));

    length = _length;
  }

  int GetFamily() const {
    return address.ss_family;
  }

  bool IsDefined() const {
    return GetFamily() != AF_UNSPEC;
  }

  void Clear() {
    address.ss_family = AF_UNSPEC;
  }

  gcc_pure
  bool operator==(const SocketAddress &other) const;

  bool operator!=(const SocketAddress &other) const {
    return !(*this == other);
  }

#ifndef _WIN32_WCE
  bool Lookup(const char *host, const char *service, int socktype);
#endif
};

#endif
