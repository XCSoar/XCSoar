// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SpectateFilePort.hpp"
#include "OpenSpectateFilePort.hpp"
#include "Device/Driver/Condor3Spectate.hpp"
#include "io/DataHandler.hpp"

#include <memory>
#include <stdexcept>

std::unique_ptr<Port>
OpenSpectateFilePort(Path path, const char *own_cn,
                     PortListener *listener,
                     DataHandler &handler)
{
  return std::make_unique<SpectateFilePort>(path, own_cn, listener, handler);
}

SpectateFilePort::SpectateFilePort(Path _path, const char *_own_cn,
                                   PortListener *listener,
                                   DataHandler &handler) noexcept
  :Port(listener, handler), SuspensibleThread("SpectateFilePort"),
   path(_path)
{
  if (_own_cn != nullptr)
    own_cn = _own_cn;
}

SpectateFilePort::~SpectateFilePort() noexcept
{
  StopRxThread();
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
  if (Thread::IsDefined()) {
    BeginStop();
    Thread::Join();
  }

  return true;
}

bool
SpectateFilePort::StartRxThread()
{
  StopRxThread();
  SuspensibleThread::Start();
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
  Condor3SpectateBuilder::Lines lines;
  const Condor3SpectateReference *ref = nullptr;
  if (device != nullptr) {
    const Condor3SpectateReference &live_ref = device->GetLiveReference();
    if (live_ref.defined)
      ref = &live_ref;
  }

  if (!Condor3SpectateBuilder::Build(path, own_cn, lines, ref))
    return;

  for (const auto &line : lines) {
    handler.DataReceived(std::span{
      reinterpret_cast<const std::byte *>(line.c_str()), line.length()});
    handler.DataReceived(std::span{
      reinterpret_cast<const std::byte *>("\r\n"), 2});
  }
}

void
SpectateFilePort::Run() noexcept
{
  Poll();

  while (!WaitForStopped(std::chrono::seconds(1)))
    Poll();
}
