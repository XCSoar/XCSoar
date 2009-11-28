/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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
 * Constructor of the MapWindowTimer class
 *
 * Resets all internal fields to the default values
 */
MapWindowTimer::MapWindowTimer():
  timestats_av(0),
  cpuload(0),
  timestats_dirty(false),
  tottime(0)
{
}

/**
 * "Starts" the timer by saving the start time in timestamp_newdata
 */
void
MapWindowTimer::StartTimer()
{
  // Saves the current tick count (time) as start time
  timestamp_newdata = ::GetTickCount();
  timestats_dirty = false;
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
  DWORD fpsTime = ::GetTickCount();
  if (fpsTime-timestamp_newdata<700) {
    // it's been less than 700 ms since last data
    // was posted
    return true;
  } else {
    return false;
  }
}

/**
 * Interrupts the timer (in case map was panned while drawing)
 */
void
MapWindowTimer::InterruptTimer()
{
  timestats_dirty = true;
  timestamp_newdata = ::GetTickCount()-700; // cause to expire
}

/**
 * "Stops" the timer by saving the stop time in timestamp_draw
 * and calculating the total drawing time.
 */
void
MapWindowTimer::StopTimer()
{
  // Saves the current tick count (time) as stop time
  timestamp_draw = ::GetTickCount();

  if (!timestats_dirty) {
    // Calculates the drawing time
    // QUESTION TB: why the weird formula and why is timestats_av = tottime?!
    tottime = (2*tottime+(timestamp_draw-timestamp_newdata))/3;
    timestats_av = tottime;
    cpuload=0;
  }

  timestats_dirty = false;
}
