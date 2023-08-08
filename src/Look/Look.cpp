// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Look.hpp"
#include "UISettings.hpp"

void
Look::Initialise(const Font &map_font)
{
  dialog.Initialise();
  traffic.Initialise(map_font);
  flarm_dialog.Initialise(traffic, false);
  gesture.Initialise();
  thermal_assistant_dialog.Initialise(false, false);
  chart.Initialise();
}

void
Look::InitialiseConfigured(const UISettings &settings,
                           const Font &map_font, const Font &map_bold_font,
                           unsigned infobox_width)
{
  dialog.Initialise();
  terminal.Initialise();
  cross_section.Initialise(map_font);
  horizon.Initialise();
  thermal_band.Initialise(settings.info_boxes.inverse,
                          cross_section.sky_color);
  trace_history.Initialise(settings.info_boxes.inverse);
  info_box.Initialise(settings.info_boxes.inverse,
                      settings.info_boxes.use_colors,
                      infobox_width);
  vario.Initialise(settings.info_boxes.inverse,
                   settings.info_boxes.use_colors,
                   infobox_width,
                   info_box.title_font);
  wind_arrow_info_box.Initialise(map_bold_font, settings.info_boxes.inverse);
  flarm_gauge.Initialise(traffic, true, settings.info_boxes.inverse);
  thermal_assistant_gauge.Initialise(true, settings.info_boxes.inverse);
  final_glide_bar.Initialise(map_bold_font);
  vario_bar.Initialise(map_bold_font);
  map.Initialise(settings.map, map_font, map_bold_font);
  icon.Initialise();
  circling_percent.Initialise();
}

void
Look::ReinitialiseLayout(unsigned infobox_width)
{
  /* dialog fonts have an upper bound depending on the window size,
     and thus they might need to be reloaded */
  dialog.LoadFonts();

  info_box.ReinitialiseLayout(infobox_width);
  vario.ReinitialiseLayout(infobox_width);
}
