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

#ifndef XCSOAR_LOGGER_FRECORD_HPP
#define XCSOAR_LOGGER_FRECORD_HPP

#include "GPSClock.hpp"

struct BrokenTime;
struct GPSState;

class LoggerFRecord
{
  GPSClock clock;
  char last_f_record[64];
  bool detect_f_record_change;

public:
  LoggerFRecord()
    :clock(fixed_270 /* 4.5 minutes */) {}

  const char *Update(const GPSState &gps, const BrokenTime &broken_time,
                     fixed time, bool nav_warning);
  void Reset();
};

#endif
