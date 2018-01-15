/*
 * Copyright (C) 2012-2015 Max Kellermann <max.kellermann@gmail.com>
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

#include "IPv4Address.hxx"
#include "Util/Macros.hpp"

#include <algorithm>

#include <assert.h>
#include <string.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#if !defined(WIN32) && !defined(__BIONIC__)
#include <ifaddrs.h>
#endif

static const struct sockaddr_in *
CastToIPv4(const struct sockaddr *p)
{
	assert(p->sa_family == AF_INET);

	/* cast through void to work around the bogus alignment warning */
	const void *q = reinterpret_cast<const void *>(p);
	return reinterpret_cast<const struct sockaddr_in *>(q);
}

IPv4Address::IPv4Address(SocketAddress src)
	:address(*CastToIPv4(src.GetAddress())) {}

#if !defined(WIN32) && !defined(__BIONIC__)

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
gcc_pure
static const struct sockaddr_in *
GetIpAddressInner(const ifaddrs *ifaddr, const char *device)
{
	/* iterate over all interfaces */
	for (const ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
		/* is this the (droid) device we're looking for and it's IPv4? */
		if (ifa->ifa_addr != nullptr && strcmp(ifa->ifa_name, device) == 0 &&
		    ifa->ifa_addr->sa_family == AF_INET)
			return CastToIPv4(ifa->ifa_addr);

	return nullptr;
}

IPv4Address
IPv4Address::GetDeviceAddress(const char *device)
{
	/* intialize result to undefined StaticSocketAddress */
	IPv4Address address;
	auto &sin = address.address;
	sin.sin_family = AF_UNSPEC;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = 0;
	std::fill_n(sin.sin_zero, ARRAY_SIZE(sin.sin_zero), 0);

	ifaddrs *ifaddr;
	if (getifaddrs(&ifaddr) == -1)
		return address;

	const struct sockaddr_in *found = GetIpAddressInner(ifaddr, device);
	if (found != nullptr)
		sin = *found;

	freeifaddrs(ifaddr);
	return address;
}

const char *
IPv4Address::ToString(char *buffer, size_t buffer_size) const
{
	if (!IsDefined())
		return nullptr;

	return inet_ntop(AF_INET, &address.sin_addr, buffer, buffer_size);
}

#endif
