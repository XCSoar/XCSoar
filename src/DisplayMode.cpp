// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DisplayMode.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "UIState.hpp"
#include "NMEA/Derived.hpp"

DisplayMode
GetNewDisplayMode(const InfoBoxSettings &settings, const UIState &ui_state,
                  const DerivedInfo &derived_info)
{
  if (ui_state.force_display_mode != DisplayMode::NONE)
    return ui_state.force_display_mode;
  else if (derived_info.circling)
    return DisplayMode::CIRCLING;
  else if (settings.use_final_glide &&
           derived_info.task_stats.flight_mode_final_glide)
    return DisplayMode::FINAL_GLIDE;
  else
    return DisplayMode::CRUISE;
}
