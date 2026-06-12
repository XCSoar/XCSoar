// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/AlertZone.hpp"
#include "FLARM/Error.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/Hardware.hpp"
#include "FLARM/State.hpp"
#include "FLARM/Progress.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/List.hpp"

#include <type_traits>

/**
 * A container for all data received by a FLARM.
 */
struct FlarmData {
  FlarmError error;

  FlarmVersion version;

  FlarmHardware hardware;

  FlarmState state;

  FlarmProgress progress;

  FlarmStatus status;

  TrafficList traffic;

  FlarmAlertZoneList alert_zones;

  constexpr bool IsDetected() const noexcept {
    return status.available || !traffic.IsEmpty();
  }

  constexpr void Clear() noexcept {
    error.Clear();
    version.Clear();
    hardware.Clear();
    state.Clear();
    progress.Clear();
    status.Clear();
    traffic.Clear();
    alert_zones.Clear();
  }

  constexpr void Complement(const FlarmData &add) noexcept {
    error.Complement(add.error);
    version.Complement(add.version);
    hardware.Complement(add.hardware);
    state.Complement(add.state);
    progress.Complement(add.progress);
    status.Complement(add.status);
    traffic.Complement(add.traffic);
    alert_zones.Complement(add.alert_zones);
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    error.Expire(clock);
    version.Expire(clock);
    hardware.Expire(clock);
    state.Expire(clock);
    progress.Expire(clock);
    status.Expire(clock);
    traffic.Expire(clock);
    alert_zones.Expire(clock);
  }
};

static_assert(std::is_trivial<FlarmData>::value, "type is not trivial");
