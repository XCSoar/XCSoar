// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <tchar.h>
#include <cstddef>

struct PixelRect;
class Menu;

namespace ButtonLabel {

struct Expanded {
  bool visible, enabled;
  const TCHAR *text;
};

[[gnu::pure]]
Expanded
Expand(const TCHAR *text, TCHAR *buffer, size_t size);

bool
ExpandMacros(const TCHAR *In, TCHAR *OutBuffer, size_t Size);

} // namespace ButtonLabel
