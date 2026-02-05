// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Base.hpp"
#include "time/BrokenDate.hpp"

class DataFieldDate final : public DataField {
  BrokenDate value;

  mutable char string_buffer[OUTBUFFERSIZE + 1];

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

  const char *GetAsString() const noexcept override;
};
