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

#ifndef IPV6_ADDRESS_HXX
#define IPV6_ADDRESS_HXX

#include "SocketAddress.hxx"
#include "OS/ByteOrder.hpp"

#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

/**
 * An OO wrapper for struct sockaddr_in.
 */
class IPv6Address {
	struct sockaddr_in6 address;

	static constexpr struct sockaddr_in6 Construct(struct in6_addr address,
						       uint16_t port,
						       uint32_t scope_id) {
		return {
#if defined(__APPLE__)
			sizeof(struct sockaddr_in6),
#endif
			AF_INET6,
			ToBE16(port),
			0,
			address,
			scope_id,
		};
	}

public:
	IPv6Address() = default;

	constexpr IPv6Address(struct in6_addr _address, uint16_t port,
			      uint32_t scope_id=0)
		:address(Construct(_address, port, scope_id)) {}

	constexpr explicit IPv6Address(uint16_t port, uint32_t scope_id=0)
		:IPv6Address(IN6ADDR_ANY_INIT, port, scope_id) {}

	/**
	 * Convert a #SocketAddress to a #IPv6Address.  Its address family must be AF_INET.
	 */
	explicit IPv6Address(SocketAddress src);

	operator SocketAddress() const {
		return SocketAddress(reinterpret_cast<const struct sockaddr *>(&address),
				     sizeof(address));
	}

	SocketAddress::size_type GetSize() {
		return sizeof(address);
	}

	constexpr bool IsDefined() const {
		return address.sin6_family != AF_UNSPEC;
	}

	constexpr bool IsValid() const {
		return address.sin6_family == AF_INET6;
	}

	constexpr uint16_t GetPort() const {
		return FromBE16(address.sin6_port);
	}

	void SetPort(uint16_t port) {
		address.sin6_port = ToBE16(port);
	}

	constexpr const struct in6_addr &GetAddress() const {
		return address.sin6_addr;
	}

	constexpr uint32_t GetScopeId() const {
		return address.sin6_scope_id;
	}

	/**
	 * Is this the IPv6 wildcard address (in6addr_any)?
	 */
	gcc_pure
	bool IsAny() const noexcept;

	/**
	 * Is this an IPv4 address mapped inside struct sockaddr_in6?
	 */
#if !GCC_OLDER_THAN(5,0) && !defined(_WIN32)
	constexpr
#endif
	bool IsV4Mapped() const noexcept {
		return IN6_IS_ADDR_V4MAPPED(&address.sin6_addr);
	}
};

#endif
