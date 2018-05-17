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

#ifndef XCSOAR_DATA_FIELD_TIME_HPP
#define XCSOAR_DATA_FIELD_TIME_HPP

#include "Base.hpp"
#include "Time/PeriodClock.hpp"

class DataFieldTime final : public DataField {
private:
  int value;
  int min;
  int max;
  unsigned step;
  unsigned max_tokens;
  PeriodClock last_step;
  uint8_t speedup;

  mutable TCHAR string_buffer[OUTBUFFERSIZE + 1];

protected:
  int SpeedUp(bool keyup);

public:
  DataFieldTime(int _min, int _max, int _value, unsigned _step,
                DataFieldListener *listener)
    :DataField(Type::TIME, true, listener),
     value(_value), min(_min), max(_max), step(_step), max_tokens(2),
     speedup(0) {}

protected:
  void SetValue(int _value) {
    if (_value == value)
      return;

    value = _value;
    Modified();
  }

public:
  void SetMin(int _min) {
    min = _min;
  }

  void SetMax(int _max) {
    max = _max;
  }

  void SetStep(unsigned _step) {
    step = _step;
  }

  void SetMaxTokenNumber(unsigned _max_tokens) {
    assert(max_tokens > 0 && max_tokens <= 4);
    max_tokens = _max_tokens;
  }

  void Set(int _value) {
    value = _value;
  }

  /* virtual methods from class DataField */
  void Inc() override;
  void Dec() override;

  int GetAsInteger() const override {
    return value;
  }

  const TCHAR *GetAsString() const override;
  const TCHAR *GetAsDisplayString() const override;

  void SetAsInteger(int _value) override {
    SetValue(_value);
  }

  ComboList CreateComboList(const TCHAR *reference) const override;
  void SetFromCombo(int data_field_index, const TCHAR *value_string) override;

protected:
  void AppendComboValue(ComboList &combo_list, int value) const;
};

#endif
