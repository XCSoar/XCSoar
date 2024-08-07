// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Base.hpp"
#include "RadioFrequency.hpp"

/**
 * This #DataField implementation stores a #RadioFrequency.
 */
class RadioFrequencyDataField final : public DataField
{
  RadioFrequency value;

  /**
   * For GetAsString().  Must be mutable because the method is const.
   */
  mutable TCHAR string_buffer[8];

public:
  explicit RadioFrequencyDataField(RadioFrequency _value,
                     DataFieldListener *listener = nullptr) noexcept
      : DataField(Type::RADIO_FREQUENCY, false, listener),
        value(_value) {}

  RadioFrequency GetValue() const noexcept
  {
    return value;
  }

  void SetValue(RadioFrequency _value) noexcept
  {
    value = _value;
  }

  void ModifyValue(RadioFrequency _value) noexcept;

  /* virtual methods from class DataField */
  const TCHAR *GetAsString() const noexcept override;
};
