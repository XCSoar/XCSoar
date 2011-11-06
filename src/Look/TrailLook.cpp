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

#include "TrailLook.hpp"
#include "SettingsMap.hpp"
#include "Screen/Ramp.hpp"
#include "Screen/Layout.hpp"

void
TrailLook::Initialise(const SETTINGS_MAP &settings_map)
{
  static gcc_constexpr_data ColorRamp snail_colors_vario[] = {
    {0,   0xc4, 0x80, 0x1e}, // sinkColor
    {100, 0xa0, 0xa0, 0xa0},
    {200, 0x1e, 0xf1, 0x73} // liftColor
  };

  static gcc_constexpr_data ColorRamp snail_colors_vario2[] = {
    {0,   0x00, 0x00, 0xff},
    {99,  0x00, 0xff, 0xff},
    {100, 0xff, 0xff, 0x00},
    {200, 0xff, 0x00, 0x00}
  };

  static gcc_constexpr_data ColorRamp snail_colors_alt[] = {
    {0,   0xff, 0x00, 0x00},
    {50,  0xff, 0xff, 0x00},
    {100, 0x00, 0xff, 0x00},
    {150, 0x00, 0xff, 0xff},
    {200, 0x00, 0x00, 0xff},
  };

  PixelScalar iwidth;
  PixelScalar minwidth = Layout::Scale(2);

  for (unsigned i = 0; i < NUMSNAILCOLORS; ++i) {
    short ih = i * 200 / (NUMSNAILCOLORS - 1);
    Color color = (settings_map.snail_type == stAltitude) ?
                  ColorRampLookup(ih, snail_colors_alt, 5) :
                  (settings_map.snail_type == stSeeYouVario) ?
                  ColorRampLookup(ih, snail_colors_vario2, 4) :
                  ColorRampLookup(ih, snail_colors_vario, 3);

    if (i < NUMSNAILCOLORS / 2 ||
        !settings_map.snail_scaling_enabled)
      iwidth = minwidth;
    else
      iwidth = max(minwidth,
                   PixelScalar((i - NUMSNAILCOLORS / 2) *
                               Layout::Scale(16) / NUMSNAILCOLORS));

    hpSnail[i].Set(minwidth, color);
    hpSnailVario[i].Set(iwidth, color);
  }

  trace_pen.Set(2, Color(50, 243, 45));
}
