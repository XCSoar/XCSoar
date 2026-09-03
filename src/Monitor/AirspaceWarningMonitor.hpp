// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NMEA/Validity.hpp"
#include "ui/event/PeriodicTimer.hpp"

/**
 * Check for new airspace warnings and show the airspace warning
 * dialog.
 */
class AirspaceWarningMonitor {
  friend class AirspaceWarningWidget;
  class AirspaceWarningWidget *widget = nullptr;

  Validity last;
  unsigned sound_interval_counter = 0;
  UI::PeriodicTimer sound_timer{[this]{ PlayRepetitiveSound(); }};

public:
  void Reset() noexcept;
  void Check() noexcept;

private:
  void PlayRepetitiveSound() noexcept;
  void HideWidget() noexcept;

  void Schedule() noexcept {
    last.Clear();
  }
};
