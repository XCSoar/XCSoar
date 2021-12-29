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

#ifndef XCSOAR_DATA_FIELD_TIME_HPP
#define XCSOAR_DATA_FIELD_TIME_HPP

#include "Base.hpp"
#include "time/PeriodClock.hpp"

class DataFieldTime final : public DataField {
private:
  std::chrono::seconds value;
  std::chrono::seconds min;
  std::chrono::seconds max;
  std::chrono::seconds step;
  unsigned max_tokens;
  PeriodClock last_step;
  uint8_t speedup;

  mutable TCHAR string_buffer[OUTBUFFERSIZE + 1];

protected:
  int SpeedUp(bool keyup) noexcept;

public:
  DataFieldTime(std::chrono::seconds _min, std::chrono::seconds _max,
                std::chrono::seconds _value, std::chrono::seconds _step,
                DataFieldListener *listener) noexcept
    :DataField(Type::TIME, true, listener),
     value(_value), min(_min), max(_max), step(_step), max_tokens(2),
     speedup(0) {}

  const auto &GetValue() const noexcept {
    return value;
  }

  void SetMin(std::chrono::seconds _min) noexcept {
    min = _min;
  }

  void SetMax(std::chrono::seconds _max) noexcept {
    max = _max;
  }

  void SetStep(std::chrono::seconds _step) noexcept {
    step = _step;
  }

  void SetMaxTokenNumber(unsigned _max_tokens) noexcept {
    assert(max_tokens > 0 && max_tokens <= 4);
    max_tokens = _max_tokens;
  }

  void SetValue(std::chrono::seconds _value) noexcept {
    value = _value;
  }

  void ModifyValue(std::chrono::seconds new_value) noexcept {
    if (new_value != GetValue()) {
      SetValue(new_value);
      Modified();
    }
  }

  /* virtual methods from class DataField */
  void Inc() noexcept override;
  void Dec() noexcept override;

  int GetAsInteger() const noexcept override {
    return value.count();
  }

  const TCHAR *GetAsString() const noexcept override;
  const TCHAR *GetAsDisplayString() const noexcept override;

  void SetAsInteger(int _value) noexcept override {
    ModifyValue(std::chrono::seconds{_value});
  }

  ComboList CreateComboList(const TCHAR *reference) const noexcept override;
  void SetFromCombo(int data_field_index, const TCHAR *value_string) noexcept override;

protected:
  void AppendComboValue(ComboList &combo_list, std::chrono::seconds value) const noexcept;
};

#endif
