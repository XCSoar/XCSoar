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
DumpPort::Write(std::span<const std::byte> src)
{
  const bool enabled = CheckEnabled();
  if (enabled)
    LogFmt("Write({})", src.size());

  std::size_t nbytes;
  try {
    nbytes = port->Write(src);
  } catch (...) {
    if (enabled)
      LogFmt("Write({})=error", src.size());
    throw;
  }

  if (enabled) {
    LogFmt("Write({})={}", src.size(), nbytes);
    HexDump("W ", src.first(nbytes));
  }

  return nbytes;
}

bool
DumpPort::Drain()
{
  if (CheckEnabled())
    LogString("Drain");

  return port->Drain();
}

void
DumpPort::Flush()
{
  if (CheckEnabled())
    LogString("Flush");

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
    LogFmt("SetBaudrate {}", baud_rate);

  port->SetBaudrate(baud_rate);
}

bool
DumpPort::StopRxThread()
{
  if (CheckEnabled())
    LogString("StopRxThread");

  return port->StopRxThread();
}

bool
DumpPort::StartRxThread()
{
  if (CheckEnabled())
    LogString("StartRxThread");

  return port->StartRxThread();
}

std::size_t
DumpPort::Read(std::span<std::byte> dest)
{
  const bool enabled = CheckEnabled();
  if (enabled)
    LogFmt("Read({})", dest.size());

  auto nbytes = port->Read(dest);

  if (enabled) {
    LogFmt("Read({})={}", dest.size(), nbytes);
    if (nbytes > 0)
      HexDump("R ", dest.first(nbytes));
  }

  return nbytes;
}

void
DumpPort::WaitRead(std::chrono::steady_clock::duration timeout)
{
  const bool enabled = CheckEnabled();
  if (enabled)
    LogFmt("WaitRead {}", timeout.count());

  try {
    port->WaitRead(timeout);
  } catch (const DeviceTimeout &) {
    if (enabled)
      LogFmt("WaitRead {} = timeout", timeout.count());
    throw;
  } catch (...) {
    if (enabled)
      LogFmt("WaitRead {} = error", timeout.count());
    throw;
  }
}
