// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Port.hpp"
#include "system/Path.hpp"
#include "ui/event/Timer.hpp"
#include "util/StaticString.hxx"

class CondorSpectateDevice;

/**
 * Poll a Condor Spectate.json file and inject FLARM NMEA into the
 * device pipeline.
 */
class SpectateFilePort final : public Port {
  const Path path;
  StaticString<128> own_cn;
  CondorSpectateDevice *device = nullptr;

  UI::Timer timer{[this]{ OnTimer(); }};
  bool running = false;

  void OnTimer() noexcept;
  void Poll() noexcept;

public:
  SpectateFilePort(Path _path, const char *_own_cn,
                   PortListener *listener, DataHandler &handler) noexcept;

  void SetDevice(CondorSpectateDevice *_device) noexcept {
    device = _device;
  }

  /* virtual methods from class Port */
  PortState GetState() const noexcept override;
  std::size_t Write(std::span<const std::byte> src) override;
  bool Drain() override;
  void Flush() override;
  unsigned GetBaudrate() const noexcept override;
  void SetBaudrate(unsigned baud_rate) override;
  bool StopRxThread() override;
  bool StartRxThread() override;
  std::size_t Read(std::span<std::byte> dest) override;

  [[noreturn]]
  void WaitRead(std::chrono::steady_clock::duration timeout) override;
};
