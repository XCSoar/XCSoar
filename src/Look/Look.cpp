/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "UISettings.hpp"

void
Look::Initialise(const Font &map_font, const Font &map_bold_font,
                 const Font &map_label_font)
{
  dialog.Initialise(map_bold_font, map_font, map_label_font,
                    map_bold_font, map_font, map_bold_font);
  traffic.Initialise(map_font);
  flarm_dialog.Initialise(traffic, false);
  gesture.Initialise();
  thermal_assistant_dialog.Initialise(false, false);
  chart.Initialise();
}

void
Look::InitialiseConfigured(const UISettings &settings,
                           const Font &map_font, const Font &map_bold_font,
                           const Font &map_label_font,
                           const Font &cdi_font,
                           const Font &monospace_font,
                           const Font &infobox_value_font,
                           const Font &infobox_small_font,
#ifndef GNAV
                           const Font &infobox_unit_font,
#endif
                           const Font &infobox_title_font)
{
  dialog.Initialise(map_bold_font, map_font, map_label_font,
                    map_bold_font, map_font, map_bold_font);
  terminal.Initialise(monospace_font);
  units.Initialise();
  vario.Initialise(settings.info_boxes.inverse,
                   settings.info_boxes.use_colors,
                   infobox_title_font, cdi_font);
  cross_section.Initialise(map_font);
  horizon.Initialise();
  thermal_band.Initialise(settings.info_boxes.inverse,
                          cross_section.sky_color);
  trace_history.Initialise(settings.info_boxes.inverse);
  info_box.Initialise(settings.info_boxes.inverse,
                      settings.info_boxes.use_colors,
                      infobox_value_font,
                      infobox_small_font,
#ifndef GNAV
                      infobox_unit_font,
#endif
                      infobox_title_font);
  wind_arrow_info_box.Initialise(map_bold_font, settings.info_boxes.inverse);
  flarm_gauge.Initialise(traffic, true, settings.info_boxes.inverse);
  thermal_assistant_gauge.Initialise(true, settings.info_boxes.inverse);
  final_glide_bar.Initialise(map_font);
  vario_bar.Initialise(map_font);
  map.Initialise(settings.map, map_font, map_bold_font);
  icon.Initialise();
}
