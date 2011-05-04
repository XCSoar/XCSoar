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

#ifndef XCSOAR_BATTERY_TIMER_HPP
#define XCSOAR_BATTERY_TIMER_HPP

#include "PeriodClock.hpp"

class BatteryTimer {
  // Battery status for SIMULATOR mode
  // 30% reminder, 20% exit, 30 second reminders on warnings

  static const unsigned BATTERY_WARNING = 30;
  static const unsigned BATTERY_EXIT = 20;
  static const unsigned BATTERY_REMINDER = 30000;

  PeriodClock last_warning;

public:
  void Process();
};

#endif
