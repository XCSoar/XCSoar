// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include <exception>

class SocketAddress;

namespace Cares {

/**
 * Handler interface for Channel::Lookup().
 */
class Handler {
public:
	/**
	 * Called once for each address.  If there are no more
	 * addresses, OnCaresSuccess() gets called.
	 */
	virtual void OnCaresAddress(SocketAddress address) noexcept = 0;
	virtual void OnCaresSuccess() noexcept = 0;
	virtual void OnCaresError(std::exception_ptr e) noexcept = 0;
};

} // namespace Cares
