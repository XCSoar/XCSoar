/* Copyright_License {

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

#ifndef AIRSPACE_ACTIVITY_HPP
#define AIRSPACE_ACTIVITY_HPP

class AirspaceActivity {
  struct Days
  {
    bool sunday:1;
    bool monday:1;
    bool tuesday:1;
    bool wednesday:1;
    bool thursday:1;
    bool friday:1;
    bool saturday:1;
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
      mask.days.sunday = true;
      break;
    case 1:
      mask.days.monday = true;
      break;
    case 2:
      mask.days.tuesday = true;
      break;
    case 3:
      mask.days.wednesday = true;
      break;
    case 4:
      mask.days.thursday = true;
      break;
    case 5:
      mask.days.friday = true;
      break;
    case 6:
      mask.days.saturday = true;
      break;
    default:
      SetAll();
      break;
    }
  }

  void SetAll() {
    mask.value = 0xFF;
  }

  void SetWeekdays() {
    mask.value = 0;
    mask.days.monday = true;
    mask.days.tuesday = true;
    mask.days.wednesday = true;
    mask.days.thursday = true;
    mask.days.friday = true;
  }

  void SetWeekend() {
    mask.value = 0;
    mask.days.saturday = true;
    mask.days.sunday = true;
  }

  bool Matches(AirspaceActivity _mask) const {
    return mask.value & _mask.mask.value;
  }
};

static_assert(sizeof(AirspaceActivity) == 1, "Wrong size");

#endif
