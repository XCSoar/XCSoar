// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Base.hpp"
#include "util/StaticString.hxx"

class DataFieldBoolean final : public DataField {
private:
  bool mValue;

  StaticString<32> true_text;
  StaticString<32> false_text;

public:
  DataFieldBoolean(bool _value,
                   const char *_true_text, const char *_false_text,
                   DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::BOOLEAN, true, listener),
     mValue(_value),
     true_text(_true_text), false_text(_false_text) {}

  bool GetValue() const noexcept {
    return mValue;
  }

  void SetValue(bool new_value) noexcept {
    mValue = new_value;
  }

  void ModifyValue(bool new_value) noexcept {
    if (new_value != GetValue()) {
      SetValue(new_value);
      Modified();
    }
  }

  /* virtual methods from class DataField */
  void Inc() noexcept override;
  void Dec() noexcept override;
  const char *GetAsString() const noexcept override;
  ComboList CreateComboList(const char *reference) const noexcept override;
  void SetFromCombo(int i, const char *s) noexcept override;
};
