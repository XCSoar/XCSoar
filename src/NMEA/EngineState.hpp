// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Validity.hpp"
#include "time/Stamp.hpp"
#include "Atmosphere/Temperature.hpp"

/** Information about engine sensors.
*/
struct EngineState
{
  /* The engine box measured revolutions per second on the camshaft. */
  Validity revs_per_sec_available;
  float revs_per_sec;

  /* The engine Cylinder Head Temperature (CHT) */
  Validity cht_temperature_available;
  Temperature cht_temperature;

  /* The engine Exhaust Gas Temperature (EGT) */
  Validity egt_temperature_available;
  Temperature egt_temperature;

  void Clear() noexcept{
    revs_per_sec_available.Clear();
    cht_temperature_available.Clear();
    egt_temperature_available.Clear();
  }

  void Reset() noexcept{
    Clear();
  }

  void Expire(TimeStamp clock) noexcept {
    revs_per_sec_available.Expire(clock, std::chrono::seconds(3));
    cht_temperature_available.Expire(clock, std::chrono::seconds(3));
    egt_temperature_available.Expire(clock, std::chrono::seconds(3));
  }

  void Complement(const EngineState &add) noexcept {
    if (revs_per_sec_available.Complement(add.revs_per_sec_available)){
      revs_per_sec = add.revs_per_sec;
    }
    if (cht_temperature_available.Complement(add.cht_temperature_available)){
      cht_temperature = add.cht_temperature;
    }
    if (egt_temperature_available.Complement(add.egt_temperature_available)){
      egt_temperature = add.egt_temperature;
    }
  }

};
