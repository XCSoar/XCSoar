// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Number.hpp"
#include "time/PeriodClock.hpp"

class DataFieldInteger final : public NumberDataField
{
  int value;
  int min;
  int max;
  int step;
  PeriodClock last_step;
  int speedup;

  mutable char output_buffer[OUTBUFFERSIZE + 1];

protected:
  int SpeedUp(bool keyup) noexcept;

public:
  DataFieldInteger(const char *edit_format, const char *display_format,
                   int _min, int _max, int _value, int _step,
                   DataFieldListener *listener=nullptr) noexcept
    :NumberDataField(Type::INTEGER,
                     /* no list picker for very big lists */
                     ((_max - _min) / _step) < 500,
                     edit_format, display_format, listener),
     value(_value), min(_min), max(_max), step(_step) {}

  void SetMin(int _min) noexcept {
    min = _min;
  }

  void SetMax(int _max) noexcept {
    max = _max;
  }

  int GetMin() const noexcept {
    return min;
  }

  int GetMax() const noexcept {
    return max;
  }

  int GetValue() const noexcept {
    return value;
  }

  void SetValue(int _value) noexcept {
    value = _value;
  }

  void ModifyValue(int new_value) noexcept {
    if (new_value != GetValue()) {
      SetValue(new_value);
      Modified();
    }
  }

  void SetAsInteger(int value) noexcept;

  /* virtual methods from class DataField */
  void Inc() noexcept override;
  void Dec() noexcept override;
  const char *GetAsString() const noexcept override;
  const char *GetAsDisplayString() const noexcept override;
  ComboList CreateComboList(const char *reference) const noexcept override;
  void SetFromCombo(int iDataFieldIndex, const char *sValue) noexcept override;

protected:
  void AppendComboValue(ComboList &combo_list, int value) const noexcept;
};
