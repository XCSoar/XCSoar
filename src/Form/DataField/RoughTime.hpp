// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Base.hpp"
#include "time/RoughTime.hpp"

/**
 * This #DataField implementation stores a UTC time of day with a
 * precision of one minute.  For displaying, it is converted to the
 * user's time zone.
 */
class RoughTimeDataField final : public DataField {
  RoughTime value;

  /**
   * This value is added when displaying the value to the user.  It is
   * the offset of the user's time zone.
   */
  RoughTimeDelta time_zone;

public:
  RoughTimeDataField(RoughTime _value, RoughTimeDelta _time_zone,
                     DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::ROUGH_TIME, false, listener),
     value(_value), time_zone(_time_zone) {}

  RoughTimeDelta GetTimeZone() const noexcept {
    return time_zone;
  }

  void SetTimeZone(RoughTimeDelta _time_zone) noexcept {
    time_zone = _time_zone;
  }

  RoughTime GetValue() const noexcept {
    return value;
  }

  void SetValue(RoughTime _value) noexcept {
    value = _value;
  }

  RoughTime GetLocalValue() const noexcept {
    return value + time_zone;
  }

  void ModifyValue(RoughTime _value) noexcept;

  /* virtual methods from class DataField */
  void Inc() noexcept override;
  void Dec() noexcept override;
  const char *GetAsString() const noexcept override;
  const char *GetAsDisplayString() const noexcept override;
};
