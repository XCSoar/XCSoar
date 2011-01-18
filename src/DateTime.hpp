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

#ifndef XCSOAR_DATE_TIME_HPP
#define XCSOAR_DATE_TIME_HPP

#include <stdint.h>

/**
 * A broken-down representation of a date.
 */
struct BrokenDate {
  /**
   * Absolute year, e.g. "2010".
   */
  uint16_t year;

  /**
   * Month number, 1-12.
   */
  uint8_t month;

  /**
   * Day of month, 1-31.
   */
  uint8_t day;

  /**
   * Day of the week (0-6, 0: sunday)
   */
  unsigned char day_of_week;
};

/**
 * A broken-down representation of a time.
 */
struct BrokenTime {
  /**
   * Hour of day, 0-23.
   */
  uint8_t hour;

  /**
   * Minute, 0-59.
   */
  uint8_t minute;

  /**
   * Second, 0-59.
   */
  uint8_t second;
};

/**
 * A broken-down representation of date and time.
 */
struct BrokenDateTime : public BrokenDate, public BrokenTime {
  /**
   * Returns the current system date and time, in UTC.
   */
  static const BrokenDateTime NowUTC();

  /**
   * Returns the current system date and time, in the current time zone.
   */
  static const BrokenDateTime NowLocal();
};

#endif
