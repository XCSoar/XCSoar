/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
