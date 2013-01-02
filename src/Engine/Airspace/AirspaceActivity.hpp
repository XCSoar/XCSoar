/* Copyright_License {

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

#ifndef AIRSPACE_ACTIVITY_HPP
#define AIRSPACE_ACTIVITY_HPP

class AirspaceActivity {
  struct Days
  {
    unsigned sunday:1;
    unsigned monday:1;
    unsigned tuesday:1;
    unsigned wednesday:1;
    unsigned thursday:1;
    unsigned friday:1;
    unsigned saturday:1;
  };

  union
  {
    Days days;
    unsigned char value;
  } mask;

public:
  AirspaceActivity() {
    SetAll();
  };

  bool equals(const AirspaceActivity _mask) const {
    return mask.value == _mask.mask.value;
  }

  AirspaceActivity(int8_t day_of_week) {
    // setter from BrokenDate format day
    mask.value = 0;
    switch(day_of_week) {
    case 0:
      mask.days.sunday = 1;
      break;
    case 1:
      mask.days.monday = 1;
      break;
    case 2:
      mask.days.tuesday = 1;
      break;
    case 3:
      mask.days.wednesday = 1;
      break;
    case 4:
      mask.days.thursday = 1;
      break;
    case 5:
      mask.days.friday = 1;
      break;
    case 6:
      mask.days.saturday = 1;
      break;
    default:
      break;
    }
  }

  void SetAll() {
    mask.value = 0xFF;
  }

  void SetWeekdays() {
    mask.value = 0;
    mask.days.monday = 1;
    mask.days.tuesday = 1;
    mask.days.wednesday = 1;
    mask.days.thursday = 1;
    mask.days.friday = 1;
  }

  void SetWeekend() {
    mask.value = 0;
    mask.days.saturday = 1;
    mask.days.sunday = 1;
  }

  bool Matches(AirspaceActivity _mask) const {
    return mask.value & _mask.mask.value;
  }
};

#endif
