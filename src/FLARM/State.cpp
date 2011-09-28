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

#include "FLARM/State.hpp"

void
FlarmState::Clear()
{
  available.Clear();
  traffic.clear();
  new_traffic = false;
}

const FLARM_TRAFFIC *
FlarmState::FindMaximumAlert() const
{
  const FLARM_TRAFFIC *alert = NULL;

  for (unsigned i = 0; i < traffic.size(); ++i) {
    const FLARM_TRAFFIC &traffic = this->traffic[i];

    if (traffic.HasAlarm() &&
        (alert == NULL ||
         (traffic.alarm_level > alert->alarm_level ||
          (traffic.alarm_level == alert->alarm_level &&
           /* if the levels match -> let the distance decide (smaller
              distance wins) */
           traffic.distance < alert->distance))))
      alert = &traffic;
  }

  return alert;
}
