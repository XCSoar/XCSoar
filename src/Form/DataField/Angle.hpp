/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "Base.hpp"
#include "Math/Angle.hpp"

/**
 * This #DataField implementation stores an angle value from 0 to 359
 * degrees.  Its precision is integer degree values.
 */
class AngleDataField final : public DataField {
  static constexpr unsigned MAX = 360u;

  unsigned value;
  unsigned step;

  /** set to true to allow adjustment of values with step/10 precision */
  bool fine;

  /**
   * For GetAsString().  Must be mutable because the method is const.
   */
  mutable TCHAR string_buffer[16];

public:
  AngleDataField(unsigned _value, unsigned _step, bool _fine,
                 DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::ANGLE, true, listener),
     value(Import(_value)), step(_step), fine(_fine) {}

  AngleDataField(int _value, unsigned _step, bool _fine,
                 DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::ANGLE, true, listener),
     value(Import(_value)), step(_step), fine(_fine) {}

  AngleDataField(Angle _value, unsigned _step, bool _fine,
                 DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::ANGLE, true, listener),
     value(Import(_value)), step(_step), fine(_fine) {}

  static constexpr unsigned Import(unsigned value) noexcept {
    return value % MAX;
  }

  [[gnu::const]]
  static unsigned Import(int value) noexcept;

  [[gnu::const]]
  static unsigned Import(Angle value) noexcept {
    return lround(value.AsBearing().Degrees()) % 360u;
  }

  Angle GetValue() const noexcept {
    return Angle::Degrees(value);
  }

  unsigned GetIntegerValue() const noexcept {
    return value;
  }

  void SetValue(unsigned _value) noexcept {
    value = Import(_value);
  }

  void SetValue(int _value) noexcept {
    value = Import(_value);
  }

  void SetValue(Angle _value) noexcept {
    value = Import(_value);
  }

  void ModifyValue(unsigned _value) noexcept;

  void ModifyValue(int _value) noexcept;
  void ModifyValue(Angle _value) noexcept;

  /* virtual methods from class DataField */
  const TCHAR *GetAsString() const noexcept override;
  const TCHAR *GetAsDisplayString() const noexcept override;

  void Inc() noexcept override;
  void Dec() noexcept override;

  ComboList CreateComboList(const TCHAR *reference) const noexcept override;
  void SetFromCombo(int i, const TCHAR *s) noexcept override;
};
