/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "TimeFormatter.hpp"
#include "DateTime.hpp"
#include "Util/StringUtil.hpp"

#include <stdio.h>
#include <stdlib.h>

void
FormatSignedTimeHHMM(TCHAR* buffer, int _time)
{
  bool negative = (_time < 0);
  const BrokenTime time = BrokenTime::FromSecondOfDayChecked(abs(_time));
  if (negative)
    _stprintf(buffer, _T("-%02u:%02u"), time.hour, time.minute);
  else
    _stprintf(buffer, _T("%02u:%02u"), time.hour, time.minute);
}

void
TimeToTextSmart(TCHAR *buffer1, TCHAR *buffer2, int _time)
{
  if ((unsigned)abs(_time) >= 24u * 3600u) {
    _tcscpy(buffer1, _T(">24h"));
    buffer2[0] = '\0';
    return;
  }

  const BrokenTime time = BrokenTime::FromSecondOfDay(abs(_time));

  if (time.hour > 0) { // hh:mm, ss
    // Set Value
    _stprintf(buffer1, _T("%02u:%02u"), time.hour, time.minute);
    _stprintf(buffer2, _T("%02u"), time.second);

  } else { // mm:ss
    _stprintf(buffer1, _T("%02u:%02u"), time.minute, time.second);
      buffer2[0] = '\0';
  }
}
