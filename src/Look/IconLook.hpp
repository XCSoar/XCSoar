// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Icon.hpp"

/**
 * This class manages the icons of various XCSoar dialogs.
 */
struct IconLook {
  // task dialog
  MaskedIcon hBmpTabTask;
  MaskedIcon hBmpTabWrench;
  MaskedIcon hBmpTabSettings;
  MaskedIcon hBmpTabCalculator;

  // status dialog
  MaskedIcon hBmpTabFlight;
  MaskedIcon hBmpTabSystem;
  MaskedIcon hBmpTabRules;
  MaskedIcon hBmpTabTimes;

  void Initialise();
};
