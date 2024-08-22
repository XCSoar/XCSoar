// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Look.hpp"
#include "UISettings.hpp"
#include "GlobalSettings.hpp"

void
Look::Initialise(const Font &map_font)
{
  dialog.Initialise();
  traffic.Initialise(map_font);
  gesture.Initialise();
  chart.Initialise();
}

[[gnu::pure]]
static bool
GetDarkMode(const UISettings &settings) noexcept
{
  switch (settings.dark_mode) {
  case UISettings::DarkMode::OFF:
    break;

  case UISettings::DarkMode::ON:
    return true;

  case UISettings::DarkMode::AUTO:
    return GlobalSettings::dark_mode;
  }

  return false;
}

void
Look::InitialiseConfigured(const UISettings &settings,
                           const Font &map_font, const Font &map_bold_font,
                           unsigned infobox_width)
{
  const bool dark_mode = GetDarkMode(settings);

  dialog.Initialise();
  terminal.Initialise();
  cross_section.Initialise(map_font);
  horizon.Initialise();
  thermal_band.Initialise(dark_mode,
                          cross_section.sky_color);
  trace_history.Initialise(dark_mode);
  info_box.Initialise(dark_mode,
                      settings.info_boxes.use_colors,
                      infobox_width);
  vario.Initialise(dark_mode,
                   settings.info_boxes.use_colors,
                   infobox_width,
                   info_box.title_font);
  wind_arrow_info_box.Initialise(map_bold_font, dark_mode);
  flarm_gauge.Initialise(traffic, true, dark_mode);
  flarm_dialog.Initialise(traffic, false, dark_mode);
  thermal_assistant_dialog.Initialise(false, dark_mode);
  thermal_assistant_gauge.Initialise(true, dark_mode);
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
