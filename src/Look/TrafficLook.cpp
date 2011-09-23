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

#include "TrafficLook.hpp"
#include "resource.h"

void
TrafficLook::Initialise()
{
  safe_color = Color(0x1d,0x9b,0xc5);
  warning_color = Color(0xfe,0x84,0x38);
  alarm_color = Color(0xfb,0x35,0x2f);

  safe_brush.set(safe_color);
  warning_brush.set(warning_color);
  alarm_brush.set(alarm_color);

  teammate_icon.load_big(IDB_TEAMMATE_POS, IDB_TEAMMATE_POS_HD);
}
