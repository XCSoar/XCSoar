// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "DeviceEmulator.hpp"
#include "Device/Util/LineSplitter.hpp"

#include <string>
#include <map>

class NMEAInputLine;

class VegaEmulator : public DeviceEmulator, PortLineSplitter {
  std::map<std::string, std::string, std::less<>> settings;

public:
  VegaEmulator() noexcept {
    handler = this;
  }

private:
  void PDVSC_S(NMEAInputLine &line) noexcept;
  void PDVSC_R(NMEAInputLine &line) noexcept;
  void PDVSC(NMEAInputLine &line) noexcept;

protected:
  bool DataReceived(std::span<const std::byte> s) noexcept override;
  bool LineReceived(const char *_line) noexcept override;
};
