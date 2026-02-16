// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "String.hpp"

/**
 * A DataField that stores a string.  The displayed string is
 * obfuscated.
 */
class PasswordDataField final : public DataFieldString {
public:
  PasswordDataField(const char *initial_value,
                    DataFieldListener *listener=nullptr) noexcept
    :DataFieldString(initial_value, listener) {}

  /* virtual methods from class DataField */
  const char *GetAsDisplayString() const noexcept override;
};
