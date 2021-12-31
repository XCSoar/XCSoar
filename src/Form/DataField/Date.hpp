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

#ifndef XCSOAR_DATA_FIELD_DATE_HPP
#define XCSOAR_DATA_FIELD_DATE_HPP

#include "Base.hpp"
#include "time/BrokenDate.hpp"

class DataFieldDate final : public DataField {
  BrokenDate value;

  mutable TCHAR string_buffer[OUTBUFFERSIZE + 1];

public:
  DataFieldDate(BrokenDate _value,  DataFieldListener *listener) noexcept
    :DataField(Type::DATE, false, listener), value(_value) { }

  const auto &GetValue() const noexcept {
    return value;
  }

  void SetValue(BrokenDate _value) noexcept {
    value = _value;
  }

  void ModifyValue(BrokenDate _value) noexcept {
    if (_value == value)
      return;

    value = _value;
    Modified();
  }

  const TCHAR *GetAsString() const noexcept override;
};

#endif  // XCSOAR_DATA_FIELD_DATE_HPP
