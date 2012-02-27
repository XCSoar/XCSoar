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

  std::fill(display, display + AIRSPACECLASSCOUNT, true);
  display[CLASSG] = false;

#ifndef ENABLE_OPENGL
  transparency = false;
#endif

  fill_mode = FillMode::DEFAULT;

#ifdef HAVE_HATCHED_BRUSH
  std::fill(brushes, brushes + AIRSPACECLASSCOUNT, 0);

  brushes[OTHER] = 2;
  brushes[CLASSA] = 3;
  brushes[CLASSB] = 3;
  brushes[CLASSC] = 3;
  brushes[CLASSD] = 3;
  brushes[CTR] = 3;
  brushes[WAVE] = 2;
  brushes[AATASK] = 3;
  brushes[CLASSE] = 3;
  brushes[CLASSF] = 3;
#endif

  std::fill(colours, colours + AIRSPACECLASSCOUNT, COLOR_RED);

  colours[OTHER] = COLOR_CYAN;
  colours[DANGER] = DarkColor(COLOR_MAGENTA);
  colours[CLASSC] = DarkColor(COLOR_MAGENTA);
  colours[CLASSD] = COLOR_BLUE;
  colours[CTR] = DarkColor(COLOR_MAGENTA);
  colours[WAVE] = DarkColor(COLOR_YELLOW);
  colours[AATASK] = COLOR_YELLOW;
  colours[CLASSE] = DarkColor(COLOR_GREEN);
  colours[CLASSF] = DarkColor(COLOR_GREEN);
}
