// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "IPv4Address.hxx"
#include "util/Macros.hpp"

#include <algorithm>

#include <cassert>
#include <string.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#if !defined(_WIN32) && !defined(__BIONIC__)
#include <ifaddrs.h>
#endif

IPv4Address::IPv4Address(SocketAddress src) noexcept
	:address(src.CastTo<struct sockaddr_in>())
{
	assert(!src.IsNull());
	assert(src.GetFamily() == AF_INET);
}

#if !defined(_WIN32) && !defined(__BIONIC__)

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
[[gnu::pure]]
static const struct sockaddr_in *
GetIpAddressInner(const ifaddrs *ifaddr, const char *device) noexcept
{
	/* iterate over all interfaces */
	for (const ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
		/* is this the (droid) device we're looking for and it's IPv4? */
		if (ifa->ifa_addr != nullptr && strcmp(ifa->ifa_name, device) == 0 &&
		    ifa->ifa_addr->sa_family == AF_INET)
			return &SocketAddress(ifa->ifa_addr, sizeof(struct sockaddr_in))
				.CastTo<struct sockaddr_in>();

	return nullptr;
}

IPv4Address
IPv4Address::GetDeviceAddress(const char *device) noexcept
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
IPv4Address::ToString(char *buffer, size_t buffer_size) const noexcept
{
	if (!IsDefined())
		return nullptr;

	return inet_ntop(AF_INET, &address.sin_addr, buffer, buffer_size);
}

#endif
