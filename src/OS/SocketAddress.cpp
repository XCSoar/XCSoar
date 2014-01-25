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

#include "SocketAddress.hpp"
#include "Util/Macros.hpp"

#include <algorithm>

#include <assert.h>
#include <string.h>

#ifdef HAVE_POSIX
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#else
#include <ws2tcpip.h>
#endif

#ifdef __GLIBC__
#include <ifaddrs.h>
#endif

bool
SocketAddress::operator==(const SocketAddress &other) const
{
  return length == other.length &&
    memcmp(&address, &other.address, length) == 0;
}

#if defined(HAVE_POSIX) && !defined(__BIONIC__)

void
SocketAddress::SetLocal(const char *path)
{
  auto &sun = reinterpret_cast<struct sockaddr_un &>(address);

  const size_t path_length = strlen(path);

  // TODO: make this a runtime check
  assert(path_length < sizeof(sun.sun_path));

  sun.sun_family = AF_LOCAL;
  memcpy(sun.sun_path, path, path_length + 1);

  /* note: Bionic doesn't provide SUN_LEN() */
  length = SUN_LEN(&sun);
}

#endif

#ifdef __GLIBC__

inline bool
SocketAddress::GetIpAddressInner(const ifaddrs *ifaddr, const char *device,
                                 char *ipaddress, const size_t ipaddress_size)
{
  /* iterate over all interfaces */
  for (const ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    /* is this the (droid) device we're looking for and it's IPv4? */
    if (ifa->ifa_addr != nullptr && strcmp(ifa->ifa_name, device) == 0 &&
        ifa->ifa_addr->sa_family == AF_INET)
      /* use getnameinfo to lookup the numeric host for this device,
         returns 0 on success */
      return getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                         ipaddress, ipaddress_size, nullptr, 0,
                         NI_NUMERICHOST) == 0;

  return false;
}

SocketAddress
SocketAddress::GetDeviceAddress(const char *device)
{
  /* intialize result to undefined SocketAddress */
  SocketAddress address;
  auto &sin = reinterpret_cast<struct sockaddr_in &>(address.address);
  sin.sin_family = AF_UNSPEC;
  sin.sin_port = 0;
  sin.sin_addr.s_addr = 0;
  std::fill_n(sin.sin_zero, ARRAY_SIZE(sin.sin_zero), 0);
  address.length = sizeof(sin);

  ifaddrs *ifaddr;
  if (getifaddrs(&ifaddr) == -1)
    return address;

  char ipaddress[INET_ADDRSTRLEN + 1];
  if (GetIpAddressInner(ifaddr, device, ipaddress, sizeof(ipaddress))) {
    sin.sin_family = AF_INET; /* set valid */
    inet_pton(AF_INET, ipaddress, &sin.sin_addr); /* to numeric */
  }

  freeifaddrs(ifaddr);
  return address;
}

const char *
SocketAddress::ToString(char *buffer, size_t buffer_size) const
{
  if (!IsDefined())
    return nullptr;

  const auto &sin = reinterpret_cast<const sockaddr_in &>(address);
  return inet_ntop(AF_INET, &sin.sin_addr, buffer, buffer_size);
}

#endif

SocketAddress
SocketAddress::MakeIPv4Port(uint32_t ip, unsigned port)
{
  SocketAddress address;
  auto &sin = reinterpret_cast<struct sockaddr_in &>(address.address);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = htonl(ip);
  std::fill_n(sin.sin_zero, ARRAY_SIZE(sin.sin_zero), 0);
  address.length = sizeof(sin);
  return address;
}

SocketAddress
SocketAddress::MakePort4(unsigned port)
{
  return MakeIPv4Port(INADDR_ANY, port);
}

#ifndef _WIN32_WCE

bool
SocketAddress::Lookup(const char *host, const char *service, int socktype)
{
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = socktype;

  struct addrinfo *ai;
  if (getaddrinfo(host, service, &hints, &ai) != 0)
    return false;

  length = ai->ai_addrlen;
  assert(length <= sizeof(address));

  memcpy(reinterpret_cast<void *>(&address),
         reinterpret_cast<void *>(ai->ai_addr), length);
  freeaddrinfo(ai);
  return true;
}

#endif
