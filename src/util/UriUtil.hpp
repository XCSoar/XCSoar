// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <string>

/**
 * Decode percent-encoded sequences (%XX) in a URI string in-place.
 * Invalid sequences (non-hex digits, truncated) are left unchanged.
 */
static inline void
PercentDecode(std::string &s) noexcept
{
  std::size_t out = 0;
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size()) {
      auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
      };
      int h = hex(s[i + 1]), l = hex(s[i + 2]);
      if (h >= 0 && l >= 0) {
        s[out++] = static_cast<char>((h << 4) | l);
        i += 2;
        continue;
      }
    }
    s[out++] = s[i];
  }
  s.resize(out);
}
