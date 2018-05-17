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

#ifndef XCSOAR_DATE_UTIL_HPP
#define XCSOAR_DATE_UTIL_HPP

#include "Compiler.h"

gcc_const
static inline bool
IsLeapYear(unsigned y)
{
    y += 1900;
    return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

gcc_const
static inline unsigned
DaysInFebruary(unsigned year)
{
  return IsLeapYear(year) ? 29 : 28;
}

gcc_const
static inline unsigned
DaysInMonth(unsigned month, unsigned year)
{
  if (month == 4 || month == 6 || month == 9 || month == 11)
    return 30;
  else if (month != 2)
    return 31;
  else
    return DaysInFebruary(year);
}

#endif
