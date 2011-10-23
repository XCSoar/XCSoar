/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include <stdio.h>

using namespace InfoBoxFactory;

void
InfoBoxSettings::Panel::Clear()
{
  name.clear();
  std::fill(contents, contents + MAX_CONTENTS, InfoBoxFactory::MIN_TYPE_VAL);
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
  inverse = false;
  use_colors = true;
  border_style = apIbBox;

  for (unsigned i = 0; i < MAX_PANELS; ++i)
    panels[i].Clear();

  static gcc_constexpr_data unsigned DFLT_CONFIG_BOXES = 9;
  static gcc_constexpr_data unsigned DFLT_CONFIG_PANELS = 4;
  static gcc_constexpr_data t_InfoBox contents[DFLT_CONFIG_PANELS][DFLT_CONFIG_BOXES] = {
    { e_WP_Name,       e_WP_Distance,     e_Thermal_Gain, e_HumidityRel, e_Temperature,
      e_H_Baro,        e_TL_Avg,          e_Fin_AltDiff,  e_Fin_TimeLocal },
    { e_WP_Name,       e_WP_Distance,     e_Bearing,      e_Act_Speed,   e_Temperature,
      e_H_Baro,        e_SpeedTaskAvg,    e_Fin_AltDiff,  e_Fin_TimeLocal },
    { e_WP_Name,       e_Fin_Distance,    e_Bearing,      e_Act_Speed,   e_WP_LD,
      e_H_Baro,        e_Fin_Time,        e_Fin_AltDiff,  e_Fin_TimeLocal },
    { e_AA_SpeedAvg,   e_Fin_AA_Distance, e_HumidityRel,  e_HeightGPS,   e_Speed_GPS,
      e_WindSpeed_Est, e_TimeLocal,       e_Load_G,       e_WindBearing_Est }
  };

  assert(MAX_PANELS >= DFLT_CONFIG_PANELS);
  assert(Panel::MAX_CONTENTS >= DFLT_CONFIG_BOXES);

  panels[0].name = N_("Circling");
  panels[1].name = N_("Cruise");
  panels[2].name = N_("FinalGlide");

  for (unsigned i = PREASSIGNED_PANELS; i < MAX_PANELS; i++)
    _stprintf(panels[i].name.buffer(), _T("AUX-%u"), i-2);

  for (unsigned i = 0; i < DFLT_CONFIG_PANELS; i++)
    for (unsigned j = 0; j < DFLT_CONFIG_BOXES; j++)
      panels[i].contents[j] = contents[i][j];
}
