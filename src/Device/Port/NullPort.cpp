// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NullPort.hpp"

#include <stdexcept>

#include <stdio.h>

NullPort::NullPort()
  :Port(nullptr, *(DataHandler *)this)
{
}

NullPort::NullPort(DataHandler &_handler)
  :Port(nullptr, _handler)
{
}

PortState
NullPort::GetState() const noexcept
{
  return PortState::READY;
}

bool
NullPort::Drain()
{
  return true;
}

void
NullPort::Flush()
{
}

std::size_t
NullPort::Write([[maybe_unused]] std::span<const std::byte> src)
{
  return src.size();
}

bool
NullPort::StopRxThread()
{
  return true;
}

bool
NullPort::StartRxThread()
{
  return true;
}

unsigned
NullPort::GetBaudrate() const noexcept
{
  return 0;
}

void
NullPort::SetBaudrate(unsigned)
{
}

std::size_t
NullPort::Read([[maybe_unused]] std::span<std::byte> dest)
{
  return 0;
}

void
NullPort::WaitRead([[maybe_unused]] std::chrono::steady_clock::duration timeout)
{
  throw std::runtime_error{"Cannot read from NullPort"};
}

bool
NullPort::DataReceived(std::span<const std::byte>) noexcept
{
  return true;
}
