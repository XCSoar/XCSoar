// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AndroidPort.hpp"
#include "Android/PortBridge.hpp"

#include <cassert>
#include <stdexcept>

AndroidPort::AndroidPort(PortListener *_listener, DataHandler &_handler,
                         PortBridge *_bridge)
  :BufferedPort(_listener, _handler), bridge(_bridge)
{
  assert(bridge != nullptr);

  bridge->setListener(Java::GetEnv(), _listener);
  bridge->setInputListener(Java::GetEnv(), this);
}

AndroidPort::~AndroidPort() noexcept
{
  assert(bridge != nullptr);

  delete bridge;
}

PortState
AndroidPort::GetState() const noexcept
{
  assert(bridge != nullptr);

  return (PortState)bridge->getState(Java::GetEnv());
}

bool
AndroidPort::Drain()
{
  assert(bridge != nullptr);

  return bridge->drain(Java::GetEnv());
}

unsigned
AndroidPort::GetBaudrate() const noexcept
{
  assert(bridge != nullptr);

  return bridge->getBaudRate(Java::GetEnv());
}

void
AndroidPort::SetBaudrate(unsigned baud_rate)
{
  assert(bridge != nullptr);

  if (!bridge->setBaudRate(Java::GetEnv(), baud_rate))
    throw std::runtime_error{"Failed to set baud rate"};
}

std::size_t
AndroidPort::Write(std::span<const std::byte> src)
{
  assert(bridge != nullptr);

  return bridge->write(Java::GetEnv(), src);
}
