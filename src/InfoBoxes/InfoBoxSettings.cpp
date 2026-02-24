// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxSettings.hpp"
#include "Language/Language.hpp"

#include <algorithm>
using namespace InfoBoxFactory;

void
InfoBoxSettings::Panel::Clear() noexcept
{
  name.clear();
  std::fill_n(contents, MAX_CONTENTS, InfoBoxFactory::MIN_TYPE_VAL);
}

bool
InfoBoxSettings::Panel::IsEmpty() const noexcept
{
  for (unsigned i = 0; i < MAX_CONTENTS; ++i)
    if (contents[i] != 0)
      return false;

  return true;
}

void
InfoBoxSettings::SetDefaults() noexcept
{
  use_final_glide = false;

  geometry = Geometry::SPLIT_8;

  use_colors = true;
  theme = Theme::FOLLOW_GLOBAL;
  border_style = BorderStyle::SHADED;

  for (unsigned i = 0; i < MAX_PANELS; ++i)
    panels[i].Clear();

  static constexpr unsigned DFLT_CONFIG_BOXES = 9;
  static constexpr unsigned DFLT_CONFIG_PANELS = 4;
  static constexpr Type contents[DFLT_CONFIG_PANELS][DFLT_CONFIG_BOXES] = {
    { e_WP_Distance,   e_TL_Avg,          NavAltitude,        e_HeightAGL,   e_TL_Gain,
      e_MacCready,     e_TL_Time,         e_Thermal_30s,      e_TimeLocal },
    { e_WP_Distance,   e_Alternate_1_GR,  NavAltitude,        e_HeightAGL,   e_WP_AltDiff,
      e_MacCready,     e_Speed_GPS,       e_GR_Avg,           e_GR_Cruise },
    { e_WP_Distance,   e_Alternate_1_GR,  NavAltitude,        e_HeightAGL,   e_Fin_AltDiff,
      e_MacCready,     e_Fin_GR,          e_GR_Avg,           e_Fin_Time },
    { e_WP_Name,       e_Fin_TimeLocal,   e_WP_Distance,      e_WP_Time,     e_Fin_Distance,
      e_Fin_Time,      e_TimeLocal,       e_TimeSinceTakeoff, e_CC_Speed }
  };

  assert(MAX_PANELS >= DFLT_CONFIG_PANELS);
  assert(Panel::MAX_CONTENTS >= DFLT_CONFIG_BOXES);

  panels[0].name = N_("Circling");
  panels[1].name = N_("Cruise");
  panels[2].name = N_("FinalGlide");

  for (unsigned i = PREASSIGNED_PANELS; i < MAX_PANELS; i++)
    panels[i].name.Format("AUX-%u", i-2);

  for (unsigned i = 0; i < DFLT_CONFIG_PANELS; i++)
    for (unsigned j = 0; j < DFLT_CONFIG_BOXES; j++)
      panels[i].contents[j] = contents[i][j];
}
