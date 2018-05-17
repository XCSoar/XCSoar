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

#ifndef XCSOAR_DATA_FIELD_BOOLEAN_HPP
#define XCSOAR_DATA_FIELD_BOOLEAN_HPP

#include "Base.hpp"
#include "Util/StaticString.hxx"

class DataFieldBoolean final : public DataField {
private:
  bool mValue;

  StaticString<32> true_text;
  StaticString<32> false_text;

public:
  DataFieldBoolean(bool _value,
                   const TCHAR *_true_text, const TCHAR *_false_text,
                   DataFieldListener *listener=nullptr)
    :DataField(Type::BOOLEAN, true, listener),
     mValue(_value),
     true_text(_true_text), false_text(_false_text) {}

  void Set(bool Value);

  bool GetAsBoolean() const {
    return mValue;
  }

  void SetAsBoolean(bool Value);

  void Set(int Value) {
    if (Value > 0)
      Set(true);
    else
      Set(false);
  }

  /* virtual methods from class DataField */
  void Inc() override;
  void Dec() override;
  int GetAsInteger() const override;
  const TCHAR *GetAsString() const override;
  void SetAsInteger(int Value) override;
  void SetAsString(const TCHAR *Value) override;
  ComboList CreateComboList(const TCHAR *reference) const override;

private:
  gcc_pure
  bool ParseString(const TCHAR *s) const;
};

#endif
