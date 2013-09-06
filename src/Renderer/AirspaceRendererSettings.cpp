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
#include "Asset.hpp"

#include <algorithm>

void
AirspaceClassRendererSettings::SetDefaults()
{
  display = true;
#ifdef HAVE_HATCHED_BRUSH
  brush = 0;
#endif
  border_color = RGB8_RED;
  fill_color = RGB8_RED;
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

#if defined(HAVE_HATCHED_BRUSH) && defined(HAVE_ALPHA_BLEND)
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

  classes[OTHER].SetColors(RGB8_CYAN);
  classes[DANGER].SetColors(RGB8_MAGENTA.Darken());
  classes[MATZ].SetColors(RGB8_MAGENTA.Darken());

  classes[AATASK].SetColors(RGB8_GREEN);

  classes[CLASSC].SetColors(RGB8_BLUE);
  classes[CLASSD].SetColors(RGB8_BLUE);

  classes[CLASSE].SetColors(RGB8_BLUE);
  classes[CLASSE].fill_mode = AirspaceClassRendererSettings::FillMode::NONE;
  classes[CLASSE].SetColors(RGB8_BLUE);
  classes[CLASSF].fill_mode = AirspaceClassRendererSettings::FillMode::NONE;

  classes[TMZ].SetColors(RGB8_GRAY);
  classes[TMZ].fill_mode = AirspaceClassRendererSettings::FillMode::NONE;

  classes[WAVE].SetColors(RGB8_YELLOW);
  classes[WAVE].border_width = 0;
  classes[WAVE].fill_mode = AirspaceClassRendererSettings::FillMode::ALL;

  classes[CTR].fill_color = RGB8_RED;
  classes[CTR].border_color = RGB8_BLUE;
  classes[TMZ].border_width = 1;
  classes[CTR].fill_mode = AirspaceClassRendererSettings::FillMode::ALL;
}
