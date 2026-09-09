// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ApplePort.hpp"
#include "Apple/PortBridge.hpp"

#include <cassert>

ApplePort::ApplePort(PortListener *_listener, DataHandler &_handler,
                     PortBridge *_bridge) noexcept
  :BufferedPort(_listener, _handler), bridge(_bridge)
{
  assert(bridge != nullptr);

  bridge->SetListener(_listener);
  bridge->SetInputListener(this);
}

ApplePort::~ApplePort() noexcept
{
  assert(bridge != nullptr);

  delete bridge;
}

PortState
ApplePort::GetState() const noexcept
{
  assert(bridge != nullptr);

  return bridge->GetState();
}

bool
ApplePort::Drain()
{
  assert(bridge != nullptr);

  return bridge->Drain();
}

unsigned
ApplePort::GetBaudrate() const noexcept
{
  return 0;
}

void
ApplePort::SetBaudrate([[maybe_unused]] unsigned baud_rate)
{
  /* a BLE GATT bridge has no configurable baud rate; accept the
     request silently, like Android's HM10Port */
}

std::size_t
ApplePort::Write(std::span<const std::byte> src)
{
  assert(bridge != nullptr);

  return bridge->Write(src);
}
