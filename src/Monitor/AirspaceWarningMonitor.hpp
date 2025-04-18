// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Validity.hpp"

/**
 * Check for new airspace warnings and show the airspace warning
 * dialog.
 */
class AirspaceWarningMonitor {
  friend class AirspaceWarningWidget;
  class AirspaceWarningWidget *widget = nullptr;

  Validity last;

public:
  void Reset() noexcept;
  void Check() noexcept;

private:
  void HideWidget() noexcept;

  void Schedule() noexcept {
    last.Clear();
  }
};
