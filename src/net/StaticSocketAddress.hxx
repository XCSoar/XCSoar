// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "SocketAddress.hxx" // IWYU pragma: export
#include "Features.hxx"

#include <cassert>
#include <string_view>

/**
 * An OO wrapper for struct sockaddr_storage.
 */
class StaticSocketAddress {
	friend class SocketDescriptor;

public:
	typedef SocketAddress::size_type size_type;

private:
	size_type size;
	struct sockaddr_storage address;

public:
	StaticSocketAddress() = default;

	explicit StaticSocketAddress(SocketAddress src) noexcept {
		*this = src;
	}

	StaticSocketAddress &operator=(SocketAddress other) noexcept;

	constexpr operator SocketAddress() const noexcept {
		return SocketAddress(*this, size);
	}

	constexpr operator struct sockaddr *() noexcept {
		return (struct sockaddr *)(void *)&address;
	}

	constexpr operator const struct sockaddr *() const noexcept {
		return (const struct sockaddr *)(const void *)&address;
	}

	/**
	 * Cast the "sockaddr" pointer to a different address type,
	 * e.g. "sockaddr_in".  This is only legal after checking
	 * GetFamily().
	 */
	template<typename T>
	constexpr const T &CastTo() const noexcept {
		/* cast through void to work around the bogus
		   alignment warning */
		const void *q = reinterpret_cast<const void *>(&address);
		return *reinterpret_cast<const T *>(q);
	}

	constexpr size_type GetCapacity() const noexcept {
		return sizeof(address);
	}

	size_type GetSize() const noexcept {
		return size;
	}

	void SetSize(size_type _size) noexcept {
		assert(_size > 0);
		assert(size_t(_size) <= sizeof(address));

		size = _size;
	}

	/**
	 * Set the size to the maximum value for this class.
	 */
	void SetMaxSize() {
		SetSize(GetCapacity());
	}

	int GetFamily() const noexcept {
		return address.ss_family;
	}

	bool IsDefined() const noexcept {
		return GetFamily() != AF_UNSPEC;
	}

	void Clear() noexcept {
		size = sizeof(address.ss_family);
		address.ss_family = AF_UNSPEC;
	}

#ifdef HAVE_UN
	/**
	 * @see SocketAddress::GetLocalRaw()
	 */
	[[gnu::pure]]
	std::string_view GetLocalRaw() const noexcept;
#endif

#ifdef HAVE_TCP
	/**
	 * Extract the port number.  Returns 0 if not applicable.
	 */
	[[gnu::pure]]
	unsigned GetPort() const noexcept {
		return ((SocketAddress)*this).GetPort();
	}

	/**
	 * @return true on success, false if this address cannot have
	 * a port number
	 */
	bool SetPort(unsigned port) noexcept;
#endif

	[[gnu::pure]]
	std::span<const std::byte> GetSteadyPart() const noexcept {
		return SocketAddress{*this}.GetSteadyPart();
	}

	[[gnu::pure]]
	bool operator==(SocketAddress other) const noexcept {
		return (SocketAddress)*this == other;
	}

	bool operator!=(SocketAddress other) const noexcept {
		return !(*this == other);
	}
};
