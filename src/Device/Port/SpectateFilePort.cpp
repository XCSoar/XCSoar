// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SpectateFilePort.hpp"
#include "Device/Driver/CondorSpectate.hpp"
#include "io/DataHandler.hpp"

#include <stdexcept>

SpectateFilePort::SpectateFilePort(Path _path, const char *_own_cn,
                                   PortListener *listener,
                                   DataHandler &handler) noexcept
  :Port(listener, handler), path(_path)
{
  if (_own_cn != nullptr)
    own_cn = _own_cn;
}

PortState
SpectateFilePort::GetState() const noexcept
{
  return PortState::READY;
}

std::size_t
SpectateFilePort::Write(std::span<const std::byte> src)
{
  return src.size();
}

bool
SpectateFilePort::Drain()
{
  return true;
}

void
SpectateFilePort::Flush()
{
}

unsigned
SpectateFilePort::GetBaudrate() const noexcept
{
  return 0;
}

void
SpectateFilePort::SetBaudrate(unsigned)
{
}

bool
SpectateFilePort::StopRxThread()
{
  running = false;
  timer.Cancel();
  return true;
}

bool
SpectateFilePort::StartRxThread()
{
  running = true;
  Poll();
  timer.Schedule(std::chrono::seconds(1));
  return true;
}

std::size_t
SpectateFilePort::Read(std::span<std::byte>)
{
  return 0;
}

[[noreturn]] void
SpectateFilePort::WaitRead(std::chrono::steady_clock::duration)
{
  throw std::runtime_error("Cannot read from SpectateFilePort");
}

void
SpectateFilePort::Poll() noexcept
{
  CondorSpectateBuilder::Lines lines;
  const CondorSpectateReference *ref = nullptr;
  if (device != nullptr) {
    const CondorSpectateReference &live_ref = device->GetLiveReference();
    if (live_ref.defined)
      ref = &live_ref;
  }

  if (!CondorSpectateBuilder::Build(path, own_cn, lines, ref))
    return;

  for (const auto &line : lines) {
    handler.DataReceived(std::span{
      reinterpret_cast<const std::byte *>(line.c_str()), line.length()});
    handler.DataReceived(std::span{
      reinterpret_cast<const std::byte *>("\r\n"), 2});
  }
}

void
SpectateFilePort::OnTimer() noexcept
{
  if (!running)
    return;

  Poll();
  timer.Schedule(std::chrono::seconds(1));
}
