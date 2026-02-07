// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Base.hpp"

#include <string>

class DataFieldString: public DataField
{
  std::string mValue;

protected:
  DataFieldString(Type _type, const TCHAR *_value,
                  DataFieldListener *listener=nullptr) noexcept
    :DataField(_type, false, listener), mValue(_value) {}

public:
  DataFieldString(const TCHAR *_value,
                  DataFieldListener *listener=nullptr) noexcept
    :DataField(Type::STRING, false, listener), mValue(_value) {}

  const TCHAR *GetValue() const noexcept {
    return mValue.c_str();
  }

  void SetValue(const TCHAR *new_value) noexcept;
  void ModifyValue(const TCHAR *new_value) noexcept;

  /* virtual methods from class DataField */
  const TCHAR *GetAsString() const noexcept override;
};
