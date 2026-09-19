// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace SystemPower {

struct Capabilities {
  bool reboot;
  bool power_off;

  constexpr bool Any() const noexcept {
    return reboot || power_off;
  }
};

Capabilities
GetCapabilities() noexcept;

bool
Reboot() noexcept;

bool
PowerOff() noexcept;

} // namespace SystemPower
