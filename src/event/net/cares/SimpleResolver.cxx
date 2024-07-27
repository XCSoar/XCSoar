// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#include "SimpleResolver.hxx"
#include "Channel.hxx"

namespace Cares {

SimpleResolver::SimpleResolver(SimpleHandler &_handler,
                               unsigned _port) noexcept : handler(_handler),
                                                          port(_port)
{
}

void
SimpleResolver::Start(Channel &channel, const char *name) noexcept
{
  channel.Lookup(name, *this, cancel_ptr);
}

} // namespace Cares
