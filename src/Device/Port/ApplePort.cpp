// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ApplePort.hpp"
#include "Apple/PortBridge.hpp"

#include <cassert>
#include <stdexcept>

ApplePort::ApplePort(PortListener *_listener, DataHandler &_handler,
                         PortBridge *_bridge)
  :BufferedPort(_listener, _handler), bridge(_bridge)
{
  assert(bridge != nullptr);

  bridge->setListener(_listener);
  bridge->setInputListener(this);
}

ApplePort::~ApplePort() noexcept
{
  assert(bridge != nullptr);
// TODO
//   delete bridge;
}

PortState
ApplePort::GetState() const noexcept
{
  assert(bridge != nullptr);

  return (PortState)bridge->getState();
}

bool
ApplePort::Drain()
{
  assert(bridge != nullptr);

  return bridge->drain();
}

unsigned
ApplePort::GetBaudrate() const noexcept
{
  assert(bridge != nullptr);

  return bridge->getBaudRate();
}

void
ApplePort::SetBaudrate(unsigned baud_rate)
{
  assert(bridge != nullptr);

  if (!bridge->setBaudRate(baud_rate))
    throw std::runtime_error{"Failed to set baud rate"};
}

std::size_t
ApplePort::Write(std::span<const std::byte> src)
{
  assert(bridge != nullptr);

  return bridge->write(src);
}
