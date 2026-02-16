// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstddef>
#include <span>

#include <tchar.h>

struct PixelRect;
class Menu;

namespace ButtonLabel {

struct Expanded {
  bool visible, enabled;
  const char *text;
};

[[gnu::pure]]
Expanded
Expand(const char *text, std::span<char> buffer) noexcept;

bool
ExpandMacros(const char *In, std::span<char> dest) noexcept;

} // namespace ButtonLabel
