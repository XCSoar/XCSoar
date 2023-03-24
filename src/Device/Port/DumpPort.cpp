// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DumpPort.hpp"
#include "Device/Error.hpp"
#include "HexDump.hpp"

#include <cstdint>
#include <stdio.h>

#ifdef __clang__
/* true, the nullptr cast below is a bad kludge */
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

DumpPort::DumpPort(std::unique_ptr<Port> _port) noexcept
  :Port(nullptr, *(DataHandler *)nullptr),
   port(std::move(_port)) {}

bool
DumpPort::CheckEnabled() noexcept
{
  if (until == std::chrono::steady_clock::time_point{})
    return false;

  if (until == std::chrono::steady_clock::time_point::max())
    return true;

  if (std::chrono::steady_clock::now() >= until) {
    /* duration has just expired; clear to avoid calling
       steady_clock::now() again in the next call */
    until = std::chrono::steady_clock::time_point{};
    return false;
  }

  return true;
}

PortState
DumpPort::GetState() const noexcept
{
  return port->GetState();
}

bool
DumpPort::WaitConnected(OperationEnvironment &env)
{
  return port->WaitConnected(env);
}

std::size_t
DumpPort::Write(const void *data, std::size_t length)
{
  const bool enabled = CheckEnabled();
  if (enabled)
    LogFormat("Write(%u)", (unsigned)length);

  std::size_t nbytes;
  try {
    nbytes = port->Write(data, length);
  } catch (...) {
    if (enabled)
      LogFormat("Write(%u)=error", (unsigned)length);
    throw;
  }

  if (enabled) {
    LogFormat("Write(%u)=%u", (unsigned)length, (unsigned)nbytes);
    HexDump("W ", data, nbytes);
  }

  return nbytes;
}

bool
DumpPort::Drain()
{
  if (CheckEnabled())
    LogFormat("Drain");

  return port->Drain();
}

void
DumpPort::Flush()
{
  if (CheckEnabled())
    LogFormat("Flush");

  port->Flush();
}

unsigned
DumpPort::GetBaudrate() const noexcept
{
  return port->GetBaudrate();
}

void
DumpPort::SetBaudrate(unsigned baud_rate)
{
  if (CheckEnabled())
    LogFormat("SetBaudrate %u", baud_rate);

  port->SetBaudrate(baud_rate);
}

bool
DumpPort::StopRxThread()
{
  if (CheckEnabled())
    LogFormat("StopRxThread");

  return port->StopRxThread();
}

bool
DumpPort::StartRxThread()
{
  if (CheckEnabled())
    LogFormat("StartRxThread");

  return port->StartRxThread();
}

std::size_t
DumpPort::Read(void *buffer, std::size_t size)
{
  const bool enabled = CheckEnabled();
  if (enabled)
    LogFormat("Read(%u)", (unsigned)size);

  auto nbytes = port->Read(buffer, size);

  if (enabled) {
    LogFormat("Read(%u)=%u", (unsigned)size, (unsigned)nbytes);
    if (nbytes > 0)
      HexDump("R ", buffer, nbytes);
  }

  return nbytes;
}

void
DumpPort::WaitRead(std::chrono::steady_clock::duration timeout)
{
  const bool enabled = CheckEnabled();
  if (enabled)
    LogFormat("WaitRead %lu", (unsigned long)timeout.count());

  try {
    port->WaitRead(timeout);
  } catch (const DeviceTimeout &) {
    if (enabled)
      LogFormat("WaitRead %lu = timeout", (unsigned long)timeout.count());
    throw;
  } catch (...) {
    if (enabled)
      LogFormat("WaitRead %lu = error", (unsigned long)timeout.count());
    throw;
  }
}
