// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

  mutable char string_buffer[OUTBUFFERSIZE + 1];

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
  const char *GetAsString() const noexcept override;
  const char *GetAsDisplayString() const noexcept override;
  ComboList CreateComboList(const char *reference) const noexcept override;
  void SetFromCombo(int data_field_index, const char *value_string) noexcept override;

protected:
  void AppendComboValue(ComboList &combo_list, std::chrono::seconds value) const noexcept;
};
