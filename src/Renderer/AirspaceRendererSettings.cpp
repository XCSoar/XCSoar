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

#include "AirspaceRendererSettings.hpp"

#include <algorithm>

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

  AirspaceClassRendererSettings default_settings = {
    true,
#ifdef HAVE_HATCHED_BRUSH
    0,
#endif
    COLOR_RED,
  };

  std::fill(classes, classes + AIRSPACECLASSCOUNT, default_settings);

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

  classes[OTHER].color = COLOR_CYAN;
  classes[DANGER].color = DarkColor(COLOR_MAGENTA);
  classes[CLASSC].color = DarkColor(COLOR_MAGENTA);
  classes[CLASSD].color = COLOR_BLUE;
  classes[CTR].color = DarkColor(COLOR_MAGENTA);
  classes[WAVE].color = DarkColor(COLOR_YELLOW);
  classes[AATASK].color = COLOR_YELLOW;
  classes[CLASSE].color = DarkColor(COLOR_GREEN);
  classes[CLASSF].color = DarkColor(COLOR_GREEN);
}
