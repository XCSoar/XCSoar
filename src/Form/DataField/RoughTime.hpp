/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_DATA_FIELD_ROUGH_TIME_HPP
#define XCSOAR_DATA_FIELD_ROUGH_TIME_HPP

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
                     DataFieldListener *listener=nullptr)
    :DataField(Type::ROUGH_TIME, false, listener),
     value(_value), time_zone(_time_zone) {}

  RoughTimeDelta GetTimeZone() const {
    return time_zone;
  }

  void SetTimeZone(RoughTimeDelta _time_zone) {
    time_zone = _time_zone;
  }

  RoughTime GetValue() const {
    return value;
  }

  void SetValue(RoughTime _value) {
    value = _value;
  }

  RoughTime GetLocalValue() const {
    return value + time_zone;
  }

  void ModifyValue(RoughTime _value);

  /* virtual methods from class DataField */
  void Inc() override;
  void Dec() override;
  int GetAsInteger() const override;
  const TCHAR *GetAsString() const override;
  const TCHAR *GetAsDisplayString() const override;
};

#endif
