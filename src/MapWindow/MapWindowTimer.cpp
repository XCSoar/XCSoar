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

#include "MapWindowTimer.hpp"

/**
 * "Starts" the timer by saving the start time in timestamp_newdata
 */
void
MapWindowTimer::StartTimer()
{
  // Saves the current tick count (time) as start time
  timestamp_newdata.update();
}

/**
 * Returns true if last call of StartTimer() is less
 * then 700ms ago
 * @return True if last call of StartTimer() is less
 * then 700ms ago, False otherwise
 */
bool
MapWindowTimer::RenderTimeAvailable()
{
  if (!timestamp_newdata.check(700))
    // it's been less than 700 ms since last data
    // was posted
    return true;

  return false;
}
