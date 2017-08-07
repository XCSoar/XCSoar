/*
 * Copyright (C) 2012-2017 Max Kellermann <max.kellermann@gmail.com>
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

#include "StaticSocketAddress.hxx"

#include <algorithm>

#include <assert.h>
#include <string.h>

#ifdef WIN32
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <netdb.h>
#endif

StaticSocketAddress &
StaticSocketAddress::operator=(SocketAddress other)
{
	size = std::min(other.GetSize(), GetCapacity());
	memcpy(&address, other.GetAddress(), size);
	return *this;
}

#ifdef HAVE_TCP

bool
StaticSocketAddress::SetPort(unsigned port)
{
	switch (GetFamily()) {
	case AF_INET:
		{
			auto &a = (struct sockaddr_in &)address;
			a.sin_port = htons(port);
			return true;
		}

	case AF_INET6:
		{
			auto &a = (struct sockaddr_in6 &)address;
			a.sin6_port = htons(port);
			return true;
		}
	}

	return false;
}

#endif

bool
StaticSocketAddress::Lookup(const char *host, const char *service, int socktype)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = socktype;

	struct addrinfo *ai;
	if (getaddrinfo(host, service, &hints, &ai) != 0)
		return false;

	size = ai->ai_addrlen;
	assert(size_t(size) <= sizeof(address));

	memcpy(reinterpret_cast<void *>(&address),
	       reinterpret_cast<void *>(ai->ai_addr), size);
	freeaddrinfo(ai);
	return true;
}
