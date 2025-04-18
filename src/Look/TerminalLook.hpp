// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Font.hpp"

struct TerminalLook {
  static constexpr Color background_color = COLOR_BLACK;
  static constexpr Color text_color = COLOR_WHITE;

  Font font;

  void Initialise();
};
