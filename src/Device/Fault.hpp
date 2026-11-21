// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/State.hpp"

enum class DeviceFaultKind {
  NONE,
  ERROR,
  NO_DATA,
};

/**
 * Persistent map-label faults.  Connecting (LIMBO) is not a fault.
 * Internal GPS is covered by the GPS labels.  Simulator ports are
 * N/A.
 */
[[gnu::pure]]
constexpr DeviceFaultKind
ClassifyDeviceFault(bool disabled, bool available, bool configured,
                    bool simulator, bool internal_gps,
                    PortState state, bool alive) noexcept
{
  if (disabled || !available || !configured || simulator || internal_gps)
    return DeviceFaultKind::NONE;

  switch (state) {
  case PortState::FAILED:
    return DeviceFaultKind::ERROR;
  case PortState::LIMBO:
    return DeviceFaultKind::NONE;
  case PortState::READY:
    return alive ? DeviceFaultKind::NONE : DeviceFaultKind::NO_DATA;
  }

  return DeviceFaultKind::NONE;
}
