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

#include "ClimbAverageCalculator.hpp"

#include <assert.h>

void
ClimbAverageCalculator::Reset()
{
  newestValIndex = -1;
  for (int i = 0; i < MAX_HISTORY; i++)
    history[i].Reset();
}

double
ClimbAverageCalculator::GetAverage(double time, double altitude,
                                   double average_time)
{
  assert(average_time <= MAX_HISTORY);

  int bestHistory;

  // Don't update newestValIndex if the time didn't move forward
  if (newestValIndex < 0 ||
      !history[newestValIndex].IsDefined() ||
      time > history[newestValIndex].time)
    newestValIndex = newestValIndex < MAX_HISTORY - 1 ? newestValIndex + 1 : 0;

  // add the new sample
  history[newestValIndex] = HistoryItem(time, altitude);

  // initially bestHistory is the current...
  bestHistory = newestValIndex;

  // now run through the history and find the best sample
  // for average period within the average time period
  for (int i = 0; i < MAX_HISTORY; i++) {
    if (!history[i].IsDefined())
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

  return 0;
}

bool
ClimbAverageCalculator::Expired(double now, double max_age) const
{
  if (newestValIndex < 0)
    return true;

  auto item = history[newestValIndex];
  if (!item.IsDefined())
    return true;

  return now < item.time || now > item.time + max_age;
}
