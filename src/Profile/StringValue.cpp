// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Map.hpp"
#include "util/UTF8.hpp"
#include "util/TruncateString.hpp"
#include "util/Macros.hpp"

bool
ProfileMap::Get(std::string_view key, std::span<char> value) const noexcept
{
  const char *src = Get(key);
  if (src == nullptr) {
    value[0] = _T('\0');
    return false;
  }

  if (!ValidateUTF8(src))
    return false;

  CopyTruncateString(value.data(), value.size(), src);
  return true;
}
