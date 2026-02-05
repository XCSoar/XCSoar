// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Password.hpp"

#include <algorithm>
#include <string_view>
#include <cstdlib>

const char *
PasswordDataField::GetAsDisplayString() const noexcept
{
  static const char *obfuscated = "********************************";
  const size_t obfuscated_length = sizeof(obfuscated) - 1;
  std::string_view s = GetAsString();
  size_t length = std::min(std::mbstowcs(nullptr, s.data(), s.size()),
    obfuscated_length);
  size_t start = obfuscated_length - length;
  return obfuscated + start;
}
