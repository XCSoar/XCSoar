// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Number.hpp"
#include "time/PeriodClock.hpp"

class DataFieldFloat : public NumberDataField {
  double mValue;
  double mMin;
  double mMax;
  double mStep;
  PeriodClock last_step;
  uint8_t mSpeedup;
  bool mFine;

  StaticString<8> unit;

  mutable char mOutBuf[OUTBUFFERSIZE+1];

protected:
  double SpeedUp(bool keyup) noexcept;

public:
  DataFieldFloat(const char *edit_format, const char *display_format,
                 double _min, double _max, double _value,
                 double _step, bool _fine,
                 DataFieldListener *listener=nullptr) noexcept
    :NumberDataField(Type::REAL, true, edit_format, display_format, listener),
     mValue(_value), mMin(_min), mMax(_max), mStep(_step),
     mSpeedup(0), mFine(_fine),
     unit("") {}

  void SetUnits(const char *text) noexcept {
    unit = text;
  }

  const char *GetUnits() const noexcept {
    return unit;
  }

  void SetMin(double v) noexcept {
    mMin = v;
  }

  void SetMax(double v) noexcept {
    mMax = v;
  }

  void SetStep(double v) noexcept {
    mStep = v;
  }

  double GetStep() const noexcept {
    return mStep;
  }

  double GetValue() const noexcept {
    return mValue;
  }

  void SetValue(double _value) noexcept {
    mValue = _value;
  }

  void ModifyValue(double Value) noexcept;

  /* virtual methods from class DataField */
  void Inc() noexcept override;
  void Dec() noexcept override;
  const char *GetAsString() const noexcept override;
  const char *GetAsDisplayString() const noexcept override;
  ComboList CreateComboList(const char *reference) const noexcept override;
  void SetFromCombo(int iDataFieldIndex, const char *sValue) noexcept override;

protected:
  /**
   * Append one value to the combo list.  Derived classes may override
   * this to annotate the entries with a derived value.
   */
  virtual void AppendComboValue(ComboList &combo_list,
                                double value) const noexcept;
};
