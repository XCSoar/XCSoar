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
#include "util/StaticString.hxx"

static constexpr unsigned EDITSTRINGSIZE = 32;

class DataFieldString: public DataField
{
  StaticString<EDITSTRINGSIZE> mValue;

protected:
  DataFieldString(Type _type, const TCHAR *_value,
                  DataFieldListener *listener=nullptr) noexcept
    :DataField(_type, false, listener), mValue(_value) {}

public:
  DataFieldString(const TCHAR *_value,
                  DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::STRING, false, listener), mValue(_value) {}

  const TCHAR *GetValue() const noexcept {
    return mValue.c_str();
  }

  void SetValue(const TCHAR *new_value) noexcept;
  void ModifyValue(const TCHAR *new_value) noexcept;

  /* virtual methods from class DataField */
  const TCHAR *GetAsString() const noexcept override;
};
