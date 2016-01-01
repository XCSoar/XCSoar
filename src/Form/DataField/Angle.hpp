/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_DATA_FIELD_ANGLE_HPP
#define XCSOAR_DATA_FIELD_ANGLE_HPP

#include "Base.hpp"
#include "Math/Angle.hpp"
#include "Compiler.h"

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

public:
  AngleDataField(unsigned _value, unsigned _step, bool _fine,
                 DataFieldListener *listener=nullptr)
    :DataField(Type::ANGLE, true, listener),
     value(Import(_value)), step(_step), fine(_fine) {}

  AngleDataField(int _value, unsigned _step, bool _fine,
                 DataFieldListener *listener=nullptr)
    :DataField(Type::ANGLE, true, listener),
     value(Import(_value)), step(_step), fine(_fine) {}

  AngleDataField(Angle _value, unsigned _step, bool _fine,
                 DataFieldListener *listener=nullptr)
    :DataField(Type::ANGLE, true, listener),
     value(Import(_value)), step(_step), fine(_fine) {}

  constexpr
  static unsigned Import(unsigned value) {
    return value % MAX;
  }

  gcc_const
  static unsigned Import(int value);

  gcc_const
  static unsigned Import(Angle value) {
    return lround(value.AsBearing().Degrees()) % 360u;
  }

  Angle GetValue() const {
    return Angle::Degrees(value);
  }

  unsigned GetIntegerValue() const {
    return value;
  }

  void SetValue(unsigned _value) {
    value = Import(_value);
  }

  void SetValue(int _value) {
    value = Import(_value);
  }

  void SetValue(Angle _value) {
    value = Import(_value);
  }

  void ModifyValue(unsigned _value);

  void ModifyValue(int _value);
  void ModifyValue(Angle _value);

  /* virtual methods from class DataField */
  int GetAsInteger() const override;
  const TCHAR *GetAsString() const override;
  const TCHAR *GetAsDisplayString() const override;

  void SetAsInteger(int value) override;
  void SetAsString(const TCHAR *value) override;

  void Inc() override;
  void Dec() override;

  ComboList CreateComboList(const TCHAR *reference) const override;
  void SetFromCombo(int i, const TCHAR *s) override;
};

#endif
