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
#include <netinet/in.h>
#include <netdb.h>
#else
#include <ws2tcpip.h>
#endif

bool
SocketAddress::operator==(const SocketAddress &other) const
{
  return length == other.length &&
    memcmp(&address, &other.address, length) == 0;
}

SocketAddress
SocketAddress::MakeIPv4Port(uint32_t ip, unsigned port)
{
  SocketAddress address;
  auto &sin = reinterpret_cast<struct sockaddr_in &>(address.address);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = htonl(ip);
  std::fill(sin.sin_zero, sin.sin_zero + ARRAY_SIZE(sin.sin_zero), 0);
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
