// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "CoLookup.hxx"
#include "Channel.hxx"

namespace Cares {

CoLookup::CoLookup(Channel &channel, const char *name, int family) noexcept
{
	channel.Lookup(name, family, *this, cancel_ptr);
}

CoLookup::CoLookup(Channel &channel, const char *name) noexcept
{
	channel.Lookup(name, *this, cancel_ptr);
}

} // namespace Cares
