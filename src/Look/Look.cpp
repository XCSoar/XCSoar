/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Look.hpp"
#include "Fonts.hpp"
#include "UISettings.hpp"

void
Look::Initialise()
{
  dialog.Initialise(Fonts::map_bold, Fonts::map, Fonts::map_label,
                    Fonts::map_bold, Fonts::map_bold);
  traffic.Initialise();
  flarm_dialog.Initialise(traffic, false);
  flarm_gauge.Initialise(traffic, true);
  gesture.Initialise();
  thermal_assistant_dialog.Initialise(false);
  thermal_assistant_gauge.Initialise(true);
}

void
Look::InitialiseConfigured(const UISettings &settings)
{
  dialog.Initialise(Fonts::map_bold, Fonts::map, Fonts::map_label,
                    Fonts::map_bold, Fonts::map_bold);
  terminal.Initialise(Fonts::monospace);
  units.Initialise();
  vario.Initialise(settings.info_boxes.inverse,
                   settings.info_boxes.use_colors,
                   Fonts::title, Fonts::cdi);
  chart.Initialise(Fonts::map, Fonts::map_label, Fonts::title);
  cross_section.Initialise();
  horizon.Initialise();
  thermal_band.Initialise(settings.info_boxes.inverse,
                          cross_section.sky_color);
  trace_history.Initialise(settings.info_boxes.inverse);
  info_box.Initialise(settings.info_boxes.inverse,
                      settings.info_boxes.use_colors);
  final_glide_bar.Initialise();
  map.Initialise(settings.map);
  icon.Initialise();
}
