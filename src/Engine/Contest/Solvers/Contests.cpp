/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Contests.hpp"
#include "util/Macros.hpp"

static const TCHAR *const contest_to_string[] = {
  _T("OLC Sprint"),
  _T("OLC FAI"),
  _T("OLC Classic"),
  _T("OLC League"),
  _T("OLC Plus"),
  _T("XContest"),
  _T("DHV-XC"),
  _T("SIS-AT"),
  _T("FFVV NetCoupe"),
  _T("DMSt"),
  _T("None"),
};

const TCHAR*
ContestToString(Contest contest) noexcept
{
  unsigned i = (unsigned)contest;
  return i < ARRAY_SIZE(contest_to_string)
    ? contest_to_string[i]
    : nullptr;
}
