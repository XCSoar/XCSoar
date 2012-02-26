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

  brushes[0] = 2;
  brushes[4] = 3;
  brushes[5] = 3;
  brushes[6] = 3;
  brushes[7] = 3;
  brushes[9] = 3;
  brushes[10] = 2;
  brushes[11] = 3;
  brushes[12] = 3;
  brushes[13] = 3;
#endif

  std::fill(colours, colours + AIRSPACECLASSCOUNT, 0);

  colours[0] = 5;
  colours[3] = 10;
  colours[6] = 10;
  colours[7] = 2;
  colours[9] = 10;
  colours[10] = 9;
  colours[11] = 3;
  colours[12] = 7;
  colours[13] = 7;
}
