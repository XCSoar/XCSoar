// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <optional>

struct MoreData;
struct DerivedInfo;
struct ComputerSettings;

/**
 * Compute target TAS minus current TAS from the latest merged sensor sample.
 *
 * Unlike DerivedInfo::V_stf, this cheap calculation is intended to run at
 * sensor/merge rate so STF audio follows total-energy and airspeed changes
 * without waiting for the next GPS calculation cycle.
 */
[[gnu::pure]]
std::optional<double>
ComputeSTFSpeedError(const MoreData &basic,
                     const DerivedInfo &calculated,
                     const ComputerSettings &settings) noexcept;
