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

#include "TrafficLook.hpp"
#include "Screen/Layout.hpp"
#include "resource.h"

void
TrafficLook::Initialise(const Font &_font)
{
  safe_color = Color(0x1d,0x9b,0xc5);
  warning_color = Color(0xfe,0x84,0x38);
  alarm_color = Color(0xfb,0x35,0x2f);

  safe_brush.Set(safe_color);
  warning_brush.Set(warning_color);
  alarm_brush.Set(alarm_color);

  UPixelScalar width = Layout::ScalePenWidth(2);
  team_pen_green.Set(width, Color(0x74, 0xFF, 0));
  team_pen_blue.Set(width, Color(0, 0x90, 0xFF));
  team_pen_yellow.Set(width, Color(0xFF, 0xE8, 0));
  team_pen_magenta.Set(width, Color(0xFF, 0, 0xCB));

  teammate_icon.LoadResource(IDB_TEAMMATE_POS, IDB_TEAMMATE_POS_HD);

  font = &_font;
}
