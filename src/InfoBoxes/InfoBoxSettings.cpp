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

#include "InfoBoxSettings.hpp"
#include "Language/Language.hpp"

#include <algorithm>
#include <tchar.h>

using namespace InfoBoxFactory;

void
InfoBoxSettings::Panel::Clear()
{
  name.clear();
  std::fill_n(contents, MAX_CONTENTS, InfoBoxFactory::MIN_TYPE_VAL);
}

bool
InfoBoxSettings::Panel::IsEmpty() const
{
  for (unsigned i = 0; i < MAX_CONTENTS; ++i)
    if (contents[i] != 0)
      return false;

  return true;
}

void
InfoBoxSettings::SetDefaults()
{
  use_final_glide = false;

  geometry = Geometry::SPLIT_8;

  inverse = false;
  use_colors = true;
  border_style = BorderStyle::BOX;

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
    panels[i].name.Format(_T("AUX-%u"), i-2);

  for (unsigned i = 0; i < DFLT_CONFIG_PANELS; i++)
    for (unsigned j = 0; j < DFLT_CONFIG_BOXES; j++)
      panels[i].contents[j] = contents[i][j];
}
