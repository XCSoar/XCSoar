// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NMEA/Info.hpp"

void
GPSState::Reset() noexcept
{
  fix_quality = FixQuality::NO_FIX;
  fix_quality_available.Clear();
  real = false;
  simulator = false;
#if defined(ANDROID) || defined(__APPLE__)
  nonexpiring_internal_gps = false;
#endif
  satellites_used_available.Clear();
  satellite_ids_available.Clear();
  hdop = -1;
  pdop = -1;
  vdop = -1;
  replay = false;
}

void
GPSState::Expire(TimeStamp now) noexcept
{
  if (fix_quality_available.Expire(now, std::chrono::seconds(5)))
    fix_quality = FixQuality::NO_FIX;

  satellites_used_available.Expire(now, std::chrono::seconds(5));
  satellite_ids_available.Expire(now, std::chrono::minutes(1));
}
