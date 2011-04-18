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

#include "ClimbAverageCalculator.hpp"

ClimbAverageCalculator::ClimbAverageCalculator():
  newestValIndex(-1) {}

void
ClimbAverageCalculator::Reset()
{
  newestValIndex = -1;
  for (int i = 0; i < MAX_HISTORY; i++)
    history[i].valid = false;
}

fixed
ClimbAverageCalculator::GetAverage(fixed time, fixed altitude, fixed average_time)
{
  assert(average_time <= fixed(MAX_HISTORY));

  int bestHistory;

  // Don't update newestValIndex if the time didn't move forward
  if (newestValIndex < 0 ||
      !history[newestValIndex].valid ||
      time > history[newestValIndex].time)
    newestValIndex = newestValIndex < MAX_HISTORY - 1 ? newestValIndex + 1 : 0;

  // add the new sample
  history[newestValIndex].valid = true;
  history[newestValIndex].time = time;
  history[newestValIndex].altitude = altitude;

  // initially bestHistory is the current...
  bestHistory = newestValIndex;

  // now run through the history and find the best sample
  // for average period within the average time period
  for (int i = 0; i < MAX_HISTORY; i++) {
    if (!history[i].valid)
      continue;

    // outside the period -> skip value
    if (history[i].time + average_time < time)
      continue;

    // is the sample older (and therefore better) than the current found ?
    if (history[i].time < history[bestHistory].time)
      bestHistory = i;
  }

  // calculate the average !
  if (bestHistory != newestValIndex)
    return (altitude - history[bestHistory].altitude) /
           (time - history[bestHistory].time);

  return fixed_zero;
}
