// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Font.hpp"

struct TopographyLook {
  Font regular_label_font;

  /** for big/medium cities */
  Font important_label_font;

  void Initialise();
};
