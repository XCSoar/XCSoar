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

#include "Look/AirspaceLook.hpp"
#include "Screen/Layout.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "resource.h"

#ifdef USE_GDI
#include "Screen/GDI/AlphaBlend.hpp"
#endif

const Color AirspaceLook::colors[] = {
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_YELLOW,
  COLOR_MAGENTA,
  COLOR_CYAN,
  DarkColor(COLOR_RED),
  DarkColor(COLOR_GREEN),
  DarkColor(COLOR_BLUE),
  DarkColor(COLOR_YELLOW),
  DarkColor(COLOR_MAGENTA),
  DarkColor(COLOR_CYAN),
  COLOR_WHITE,
  COLOR_LIGHT_GRAY,
  COLOR_GRAY,
  COLOR_BLACK,
};

void
AirspaceLook::Initialise(const AirspaceRendererSettings &settings)
{
  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++)
    pens[i].Set(Layout::Scale(2),
                colors[settings.colours[i]]);

  // airspace brushes and colors
#ifdef HAVE_HATCHED_BRUSH
  bitmaps[0].load(IDB_AIRSPACE0);
  bitmaps[1].load(IDB_AIRSPACE1);
  bitmaps[2].load(IDB_AIRSPACE2);
  bitmaps[3].load(IDB_AIRSPACE3);
  bitmaps[4].load(IDB_AIRSPACE4);
  bitmaps[5].load(IDB_AIRSPACE5);
  bitmaps[6].load(IDB_AIRSPACE6);
  bitmaps[7].load(IDB_AIRSPACE7);

  for (int i = 0; i < NUMAIRSPACEBRUSHES; i++)
    brushes[i].Set(bitmaps[i]);
#endif

#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
#endif
#if defined(HAVE_ALPHA_BLEND) || !defined(HAVE_HATCHED_BRUSH)
    for (unsigned i = 0; i < NUMAIRSPACECOLORS; ++i)
      solid_brushes[i].Set(colors[i]);
#endif

  intercept_icon.load_big(IDB_AIRSPACEI, IDB_AIRSPACEI_HD);
}
