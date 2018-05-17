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

#ifndef XCSOAR_BATTERY_TIMER_HPP
#define XCSOAR_BATTERY_TIMER_HPP

#include "Time/PeriodClock.hpp"

class BatteryTimer {
  // Battery status for SIMULATOR mode
  // 10% reminder, 5% exit, 5 minute reminders on warnings

  static constexpr unsigned BATTERY_WARNING = 10;
  static constexpr unsigned BATTERY_EXIT = 5;
  static constexpr unsigned BATTERY_REMINDER = 5 * 60 * 1000;

  PeriodClock last_warning;

public:
  void Process();
};

#endif
