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

struct ifaddrs;

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

#ifdef __GLIBC__
private:
  /**
   * helper to iterate over available devices, locate the
   * passed through device name, if found write IP address in
   * provided IP address buffer
   *
   * @param ifaddr is a properly initialized interface address list
   * @param device is the name of the device we're looking for
   * @param ipaddress is a pointer to the buffer to receive the IP address (if found)
   * @param ipaddress_size is the size of the ipaddress buffer
   * @return true on success
   */
  static bool GetIpAddressInner(const ifaddrs *ifaddr, const char *device,
                                char *ipaddress, size_t ipaddress_size);

public:
  /**
   * Returns a SocketAddress for the specified device. Caller
   * should check for validity of returned SocketAddress.
   *
   * @param device is the device name f.i. "eth0"
   * @return SocketAddress, use IsDefined() to check valid result
   */
  gcc_pure
  static SocketAddress GetDeviceAddress(const char *device);

  /**
   * Converts SocketAddress to human readable string
   *
   * @param buffer is the result buffer
   * @param buffer_size is the buffer size
   * @return IP address on success, else NULL
   */
  gcc_pure
  const char *ToString(char *buffer, size_t buffer_size) const;
#endif

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
