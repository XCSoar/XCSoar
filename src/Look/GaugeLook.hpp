// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once
#include "ui/canvas/Font.hpp"

class Font;

struct GaugeLook {
  Font no_data_font;

  // void Initialise();
  void Initialise(const Font &_font);
};
