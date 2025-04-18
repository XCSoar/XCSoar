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
  const TCHAR *text;
};

[[gnu::pure]]
Expanded
Expand(const TCHAR *text, std::span<TCHAR> buffer) noexcept;

bool
ExpandMacros(const TCHAR *In, std::span<TCHAR> dest) noexcept;

} // namespace ButtonLabel
