// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/AirspaceClass.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"

/**
 * FIR-class volumes: outline-only at overview.  Checks both AY and
 * AC so OpenAIP files that put FIR on either tag still match.
 */
[[gnu::const]]
inline bool
AirspaceIsLargeFeature(AirspaceClass c) noexcept
{
  switch (c) {
  case FIR:
  case UIR:
  case LTA:
  case UTA:
  case ACC_SECTOR:
    return true;
  default:
    return false;
  }
}

[[gnu::pure]]
inline bool
AirspaceIsLargeFeature(const AbstractAirspace &airspace) noexcept
{
  return AirspaceIsLargeFeature(airspace.GetType()) ||
    AirspaceIsLargeFeature(airspace.GetClass());
}

/**
 * Outline only: screen-covering volume, or large-feature class at
 * overview.  Warnings keep a fill.
 */
[[gnu::const]]
inline bool
AirspaceLodOutlineOnly(unsigned lod, bool large_feature,
                       bool covers_view, bool has_warning) noexcept
{
  if (has_warning)
    return false;
  if (covers_view)
    return true;
  return lod >= 4 && large_feature;
}
