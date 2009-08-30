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

#include "Formatter/Time.hpp"
#include "LocalTime.hpp"
#include "Blackboard.hpp"
#include "SettingsTask.hpp"
#include <stdlib.h>

void FormatterTime::SecsToDisplayTime(int d) {
  bool negative = (d<0);
  int dd = abs(d) % (3600*24);

  hours = (dd/3600);
  mins = (dd/60-hours*60);
  seconds = (dd-mins*60-hours*3600);
  hours = hours % 24;
  if (negative) {
    if (hours>0) {
      hours = -hours;
    } else if (mins>0) {
      mins = -mins;
    } else {
      seconds = -seconds;
    }
  }
  Valid = true;
}


void FormatterTime::AssignValue(int i) {
  switch (i) {
  case 9:
    SecsToDisplayTime((int)CALCULATED_INFO.LastThermalTime);
    break;
  case 36:
    SecsToDisplayTime((int)CALCULATED_INFO.FlightTime);
    break;
  case 39:
    SecsToDisplayTime(DetectCurrentTime());
    break;
  case 40:
    SecsToDisplayTime((int)(GPS_INFO.Time));
    break;
  case 46:
    SecsToDisplayTime((int)(CALCULATED_INFO.LegTimeToGo+DetectCurrentTime()));
    Valid = ValidTaskPoint(ActiveWayPoint) &&
      (CALCULATED_INFO.LegTimeToGo< 0.9*ERROR_TIME);
    break;
  default:
    break;
  }
}


void FormatterAATTime::AssignValue(int i) {
  double dd;
  if (AATEnabled && ValidTaskPoint(ActiveWayPoint)) {
    dd = CALCULATED_INFO.TaskTimeToGo;
    if ((CALCULATED_INFO.TaskStartTime>0.0) && (CALCULATED_INFO.Flying)
        &&(ActiveWayPoint>0)) {
      dd += GPS_INFO.Time-CALCULATED_INFO.TaskStartTime;
    }
    dd= max(0,min(24.0*3600.0,dd))-AATTaskLength*60;
    if (dd<0) {
      status = 1; // red
    } else {
      if (CALCULATED_INFO.TaskTimeToGoTurningNow > (AATTaskLength+5)*60) {
        status = 2; // blue
      } else {
        status = 0;  // black
      }
    }
  } else {
    dd = 0;
    status = 0; // black
  }

  switch (i) {
  case 27:
    SecsToDisplayTime((int)CALCULATED_INFO.AATTimeToGo);
    Valid = (ValidTaskPoint(ActiveWayPoint) && AATEnabled
	     && (CALCULATED_INFO.AATTimeToGo< 0.9*ERROR_TIME));
    break;
  case 41:
    SecsToDisplayTime((int)(CALCULATED_INFO.TaskTimeToGo));
    Valid = ValidTaskPoint(ActiveWayPoint)
      && (CALCULATED_INFO.TaskTimeToGo< 0.9*ERROR_TIME);
    break;
  case 42:
    SecsToDisplayTime((int)(CALCULATED_INFO.LegTimeToGo));
    Valid = ValidTaskPoint(ActiveWayPoint)
      && (CALCULATED_INFO.LegTimeToGo< 0.9*ERROR_TIME);
    break;
  case 45:
    SecsToDisplayTime((int)(CALCULATED_INFO.TaskTimeToGo+DetectCurrentTime()));
    Valid = ValidTaskPoint(ActiveWayPoint)
      && (CALCULATED_INFO.TaskTimeToGo< 0.9*ERROR_TIME);
    break;
  case 62:
    if (AATEnabled && ValidTaskPoint(ActiveWayPoint)) {
      SecsToDisplayTime((int)dd);
      Valid = (dd< 0.9*ERROR_TIME);
    } else {
      SecsToDisplayTime(0);
      Valid = false;
    }
    break;
  default:
    break;
  }
}

const TCHAR *FormatterTime::Render(int *color) {
  if (!Valid) {
    RenderInvalid(color);
    _stprintf(Text,_T("--:--"));
  } else {
    if ((hours<0) || (mins<0) || (seconds<0)) {
      // Time is negative
      *color = 1; // red!
      if (hours<0) { // hh:mm, ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  _T("%02d"),
                  seconds);
      } else if (mins<0) { // mm:ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, _T(""));
      } else {
        _stprintf(Text,
                  _T("-00:%02d"),
                  abs(seconds));
        _tcscpy(CommentText, _T(""));
      }
    } else {
      // Time is positive
      *color = 0; // black
      if (hours>0) { // hh:mm, ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  _T("%02d"),
                  seconds);
      } else { // mm:ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, _T(""));
      }
    }
  }
  return(Text);
}


const TCHAR *FormatterAATTime::Render(int *color) {
  if (!Valid) {
    RenderInvalid(color);
    _stprintf(Text,_T("--:--"));
  } else {

    *color = status;

    if ((hours<0) || (mins<0) || (seconds<0)) {
      // Time is negative
      if (hours<0) { // hh:mm, ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  _T("%02d"),
                  seconds);
      } else if (mins<0) { // mm:ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, _T(""));
      } else {
        _stprintf(Text,
                  _T("-00:%02d"),
                  abs(seconds));
        _tcscpy(CommentText, _T(""));
      }
    } else {
      // Time is positive
      if (hours>0) { // hh:mm, ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  hours, mins );
        _stprintf(CommentText,
                  _T("%02d"),
                  seconds);
      } else { // mm:ss
        _stprintf(Text,
                  _T("%02d:%02d"),
                  mins, seconds );
        _tcscpy(CommentText, _T(""));
      }
    }
  }
  return(Text);
}

