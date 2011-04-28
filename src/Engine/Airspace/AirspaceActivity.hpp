/* Copyright_License {

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
#ifndef AIRSPACE_ACTIVITY_HPP
#define AIRSPACE_ACTIVITY_HPP

class AirspaceActivity {
  struct days
  {
    unsigned sunday:1;
    unsigned monday:1;
    unsigned tuesday:1;
    unsigned wednesday:1;
    unsigned thursday:1;
    unsigned friday:1;
    unsigned saturday:1;
  };

  union untype
  {
    days un_bitstruct;
    unsigned char bits;
  } v;

public:
  AirspaceActivity() {
    set_all();
  };

  bool equals(const AirspaceActivity mask) const {
    return v.bits == mask.v.bits;
  }

  AirspaceActivity(const unsigned char day_of_week) {
    // setter from BrokenDate format day
    v.bits = 0;
    switch(day_of_week) {
    case 0:
      v.un_bitstruct.sunday = 1;
      break;
    case 1:
      v.un_bitstruct.monday = 1;
      break;
    case 2:
      v.un_bitstruct.tuesday = 1;
      break;
    case 3:
      v.un_bitstruct.wednesday = 1;
      break;
    case 4:
      v.un_bitstruct.thursday = 1;
      break;
    case 5:
      v.un_bitstruct.friday = 1;
      break;
    case 6:
      v.un_bitstruct.saturday = 1;
      break;
    default:
      break;
    }
  }

  void set_all() {
    v.bits = 0xFF;
  }

  void set_weekdays() {
    v.bits = 0;
    v.un_bitstruct.monday = 1;
    v.un_bitstruct.tuesday = 1;
    v.un_bitstruct.wednesday = 1;
    v.un_bitstruct.thursday = 1;
    v.un_bitstruct.friday = 1;
  }

  void set_weekend() {
    v.bits = 0;
    v.un_bitstruct.saturday = 1;
    v.un_bitstruct.sunday = 1;
  }

  bool matches(AirspaceActivity mask) const {
    return (v.bits) && (mask.v.bits);
  }
};

#endif
