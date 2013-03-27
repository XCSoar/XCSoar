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

#include "AirspaceRendererSettings.hpp"

#include <algorithm>

void
AirspaceClassRendererSettings::SetDefaults()
{
  display = true;
#ifdef HAVE_HATCHED_BRUSH
  brush = 0;
#endif
  border_color = COLOR_RED;
  fill_color = COLOR_RED;
  border_width = 2;
  fill_mode = FillMode::PADDING;
}

void
AirspaceRendererSettings::SetDefaults()
{
  enable = true;
  black_outline = false;
  altitude_mode = AirspaceDisplayMode::ALLON;
  clip_altitude = 1000;

#ifndef ENABLE_OPENGL
  transparency = false;
#endif

  fill_mode = FillMode::DEFAULT;

  for (auto it = classes; it != classes + AIRSPACECLASSCOUNT; ++it)
    it->SetDefaults();

  classes[CLASSG].display = false;

#ifdef HAVE_HATCHED_BRUSH
  classes[OTHER].brush = 2;
  classes[CLASSA].brush = 3;
  classes[CLASSB].brush = 3;
  classes[CLASSC].brush = 3;
  classes[CLASSD].brush = 3;
  classes[CTR].brush = 3;
  classes[WAVE].brush = 2;
  classes[AATASK].brush = 3;
  classes[CLASSE].brush = 3;
  classes[CLASSF].brush = 3;
#endif

  classes[OTHER].SetColors(COLOR_CYAN);
  classes[DANGER].SetColors(DarkColor(COLOR_MAGENTA));
  classes[MATZ].SetColors(DarkColor(COLOR_MAGENTA));

  classes[AATASK].SetColors(Color(0x00, 0xFF, 0x00));

  classes[CLASSC].SetColors(COLOR_BLUE);
  classes[CLASSD].SetColors(COLOR_BLUE);

  classes[CLASSE].SetColors(Color(0x00, 0x00, 0xFF));
  classes[CLASSE].fill_mode = AirspaceClassRendererSettings::FillMode::NONE;
  classes[CLASSE].SetColors(Color(0x00, 0x00, 0xFF));
  classes[CLASSF].fill_mode = AirspaceClassRendererSettings::FillMode::NONE;

  classes[TMZ].SetColors(Color(0x80, 0x80, 0x80));
  classes[TMZ].fill_mode = AirspaceClassRendererSettings::FillMode::NONE;

  classes[WAVE].SetColors(Color(0xFF, 0xFF, 0x00));
  classes[WAVE].border_width = 0;
  classes[WAVE].fill_mode = AirspaceClassRendererSettings::FillMode::ALL;

  classes[CTR].fill_color = Color(0xFF, 0x00, 0x00);
  classes[CTR].border_color = Color(0x00, 0x00, 0xFF);
  classes[TMZ].border_width = 1;
  classes[CTR].fill_mode = AirspaceClassRendererSettings::FillMode::ALL;
}
