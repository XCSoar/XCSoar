// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Base.hpp"
#include "Math/Angle.hpp"

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

  /**
   * For GetAsString().  Must be mutable because the method is const.
   */
  mutable char string_buffer[16];

public:
  AngleDataField(unsigned _value, unsigned _step, bool _fine,
                 DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::ANGLE, true, listener),
     value(Import(_value)), step(_step), fine(_fine) {}

  AngleDataField(int _value, unsigned _step, bool _fine,
                 DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::ANGLE, true, listener),
     value(Import(_value)), step(_step), fine(_fine) {}

  AngleDataField(Angle _value, unsigned _step, bool _fine,
                 DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::ANGLE, true, listener),
     value(Import(_value)), step(_step), fine(_fine) {}

  static constexpr unsigned Import(unsigned value) noexcept {
    return value % MAX;
  }

  [[gnu::const]]
  static unsigned Import(int value) noexcept;

  [[gnu::const]]
  static unsigned Import(Angle value) noexcept {
    return lround(value.AsBearing().Degrees()) % 360u;
  }

  Angle GetValue() const noexcept {
    return Angle::Degrees(value);
  }

  unsigned GetIntegerValue() const noexcept {
    return value;
  }

  void SetValue(unsigned _value) noexcept {
    value = Import(_value);
  }

  void SetValue(int _value) noexcept {
    value = Import(_value);
  }

  void SetValue(Angle _value) noexcept {
    value = Import(_value);
  }

  void ModifyValue(unsigned _value) noexcept;

  void ModifyValue(int _value) noexcept;
  void ModifyValue(Angle _value) noexcept;

  /* virtual methods from class DataField */
  const char *GetAsString() const noexcept override;
  const char *GetAsDisplayString() const noexcept override;

  void Inc() noexcept override;
  void Dec() noexcept override;

  ComboList CreateComboList(const char *reference) const noexcept override;
  void SetFromCombo(int i, const char *s) noexcept override;
};
