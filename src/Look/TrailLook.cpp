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

#include "TrailLook.hpp"
#include "MapSettings.hpp"
#include "Screen/Ramp.hpp"
#include "Screen/Layout.hpp"
#include "Util/Macros.hpp"

#include <algorithm>

static RGB8Color
GetVario1Color(short ramp_h) {
  static constexpr ColorRamp snail_colors_vario[] = {
    {  0, { 0xc4, 0x80, 0x1e }}, // sinkColor
    {100, { 0xa0, 0xa0, 0xa0 }},
    {200, { 0x1e, 0xf1, 0x73 }} // liftColor
  };

  return ColorRampLookup(ramp_h, snail_colors_vario,
                         ARRAY_SIZE(snail_colors_vario));
}

static RGB8Color
GetVario2Color(short ramp_h) {
  static constexpr ColorRamp snail_colors_vario2[] = {
    {  0, { 0x00, 0x00, 0xff }},
    { 99, { 0x00, 0xff, 0xff }},
    {100, { 0xff, 0xff, 0x00 }},
    {200, { 0xff, 0x00, 0x00 }}
  };

  return ColorRampLookup(ramp_h, snail_colors_vario2,
                         ARRAY_SIZE(snail_colors_vario2));
}

static RGB8Color
GetAltitudeColor(short ramp_h) {
  static constexpr ColorRamp snail_colors_alt[] = {
    {  0, { 0xff, 0x00, 0x00 }},
    { 50, { 0xff, 0xff, 0x00 }},
    {100, { 0x00, 0xff, 0x00 }},
    {150, { 0x00, 0xff, 0xff }},
    {200, { 0x00, 0x00, 0xff }},
  };

  return ColorRampLookup(ramp_h, snail_colors_alt,
                         ARRAY_SIZE(snail_colors_alt));
}

gcc_const
static RGB8Color
GetPortableColor(TrailSettings::Type type, short ramp_h)
{
  switch (type) {
  case TrailSettings::Type::ALTITUDE:
    return GetAltitudeColor(ramp_h);
  case TrailSettings::Type::VARIO_2:
  case TrailSettings::Type::VARIO_2_DOTS:
  case TrailSettings::Type::VARIO_DOTS_AND_LINES:
    return GetVario2Color(ramp_h);
  default:
    return GetVario1Color(ramp_h);
  }
}

gcc_const
static Color
GetColor(TrailSettings::Type type, short ramp_h)
{
  return Color(GetPortableColor(type, ramp_h));
}

void
TrailLook::Initialise(const TrailSettings &settings)
{
  unsigned iwidth;
  unsigned minwidth = Layout::ScalePenWidth(2);

  for (unsigned i = 0; i < NUMSNAILCOLORS; ++i) {
    short ih = i * 200 / (NUMSNAILCOLORS - 1);
    Color color = GetColor(settings.type, ih);

    if (i < NUMSNAILCOLORS / 2 ||
        !settings.scaling_enabled)
      iwidth = minwidth;
    else
      iwidth = std::max(minwidth,
                        (i - NUMSNAILCOLORS / 2) *
                        Layout::ScalePenWidth(16u) / NUMSNAILCOLORS);

    trail_widths[i] = iwidth;
    trail_brushes[i].Create(color);
    trail_pens[i].Create(minwidth, color);
    scaled_trail_pens[i].Create(iwidth, color);
  }

  trace_pen.Create(minwidth, Color(50, 243, 45));
}
