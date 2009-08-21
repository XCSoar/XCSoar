/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "Formatter/TeamCode.hpp"
#include "externs.h"

const TCHAR *FormatterTeamCode::Render(int *color) {

  if(ValidWayPoint(TeamCodeRefWaypoint))
    {
      *color = 0; // black text
       _tcsncpy(Text,CALCULATED_INFO.OwnTeamCode,5);
       Text[5] = '\0';
    }
  else
    {
      RenderInvalid(color);
    }

  return(Text);
}


const TCHAR *FormatterDiffTeamBearing::Render(int *color) {

  if(ValidWayPoint(TeamCodeRefWaypoint) && TeammateCodeValid) {
    Valid = true;

    Value = CALCULATED_INFO.TeammateBearing -  GPS_INFO.TrackBearing;

    if (Value < -180.0)
      Value += 360.0;
    else
      if (Value > 180.0)
        Value -= 360.0;

#ifndef __MINGW32__
    if (Value > 1)
      _stprintf(Text, TEXT("%2.0f°»"), Value);
    else if (Value < -1)
      _stprintf(Text, TEXT("«%2.0f°"), -Value);
    else
      _tcscpy(Text, TEXT("«»"));
#else
    if (Value > 1)
      _stprintf(Text, TEXT("%2.0fÂ°Â»"), Value);
    else if (Value < -1)
      _stprintf(Text, TEXT("Â«%2.0fÂ°"), -Value);
    else
      _tcscpy(Text, TEXT("Â«Â»"));
#endif
    *color = 0;

  } else {
    Valid = false;
    RenderInvalid(color);
  }

  return(Text);
}


