/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Formatter/WayPoint.hpp"
#include "XCSoar.h"
#include "Protection.hpp"
#include "SettingsComputer.hpp" // for auto-setting of alternates.  Dangerous!
#include "SettingsUser.hpp"
#include "Components.hpp"

#include <stdio.h>
#include "Interface.hpp"

#include "Task/TaskManager.hpp"

const TCHAR *FormatterWaypoint::Render(int *color) {

  if (TaskPoint* tp = task_manager.getActiveTaskPoint()) {
    const Waypoint& way_point = tp->get_waypoint();

    if (Calculated().task_stats.current_leg.solution_remaining.is_final_glide()) {
      *color = 2; // blue text
    } else {
      *color = 0; // black text
    }
    if ( SettingsMap().DisplayTextType == DISPLAYFIRSTTHREE)
    {
      _tcsncpy(Text, way_point.Name.c_str(),3);
      Text[3] = '\0';
    }
    else if( SettingsMap().DisplayTextType == DISPLAYNUMBER)
    {
      _stprintf(Text,_T("%d"), way_point.id );
    }
    else
    {
      _tcsncpy(Text, way_point.Name.c_str(),
               (sizeof(Text)/sizeof(TCHAR))-1);
      Text[(sizeof(Text)/sizeof(TCHAR))-1] = '\0';
    }
  }
  else
  {
    Valid = false;
    RenderInvalid(color);
  }
  return(Text);
}

// VENTA3 Alternate destinations
const TCHAR *FormatterAlternate::RenderTitle(int *color) {
#ifdef OLD_TASK
  if(way_points.verify_index(ActiveAlternate)) {
    const WAYPOINT &way_point = way_points.get(ActiveAlternate);

    if ( SettingsMap().DisplayTextType == DISPLAYFIRSTTHREE)
    {
      _tcsncpy(Text, way_point.Name,3);
      Text[3] = '\0';
    } else if( SettingsMap().DisplayTextType == DISPLAYNUMBER) {
      _stprintf(Text, _T("%d"), way_point.Number);
    } else {
      _tcsncpy(Text, way_point.Name,
               (sizeof(Text)/sizeof(TCHAR))-1);
      Text[(sizeof(Text)/sizeof(TCHAR))-1] = '\0';
    }
  } else {
    Valid = false;
    RenderInvalid(color);
  }
#endif
  return(Text);
}


/*
 * Currently even if set for FIVV, colors are not used.
 */
const TCHAR *
FormatterAlternate::Render(int *color)
{
#ifdef OLD_TASK
  if(Valid && way_points.verify_index(ActiveAlternate)) {
    const WPCALC &wpcalc = way_points.get_calc(ActiveAlternate);

    switch (wpcalc.VGR) {
    case 0:
      // impossible, give a magenta debug color;
      *color = 5;
      break;
    case 1:
#ifdef FIVV
      *color = 0; // green
#else
      *color = 0; // blue
#endif
      break;
    case 2:
#ifdef FIVV
      *color = 0; // yellow 4
#else
      *color = 0; // normale white
#endif
      break;
    case 3:
      *color = 1; // red
      break;
    default:
      // even more impossible, give debug color magenta
      *color = 5;
      break;
    }

    Value = wpcalc.GR;

    _stprintf(Text,Format,Value);
  } else {
    Valid = false;
    RenderInvalid(color);
  }
#endif
  return(Text);
}


void FormatterAlternate::AssignValue(int i) {
  switch (i) {
  case 67:
    if (!SettingsComputer().EnableAlternate1) {
      // first run, activate calculations
      SetSettingsComputer().EnableAlternate1 = true;
      Value=INVALID_GR;
    } else {
#ifdef OLD_TASK
      if ( way_points.verify_index(SettingsComputer().Alternate1) )
        Value = way_points.get_calc(SettingsComputer().Alternate1).GR;
      else
        Value=INVALID_GR;
#endif
    }
    break;
    /*
      if ( way_points.verify_index(Alternate1) ) Value=WayPointCalc[Alternate1].GR;
      else Value=INVALID_GR;
      break;
    */
  case 68:
    if (!SettingsComputer().EnableAlternate2) { // first run, activate calculations
      SetSettingsComputer().EnableAlternate2 = true;
      Value=INVALID_GR;
    } else {
#ifdef OLD_TASK
      if ( way_points.verify_index(SettingsComputer().Alternate2) )
        Value = way_points.get_calc(SettingsComputer().Alternate2).GR;
      else Value=INVALID_GR;
#endif
     }
    break;
  case 69:
    if (!SettingsComputer().EnableBestAlternate) {
      // first run, waiting for slowcalculation loop
      SetSettingsComputer().EnableBestAlternate = true;	  // activate it
      Value=INVALID_GR;
    } else {
#ifdef OLD_TASK
      if ( way_points.verify_index(Calculated().BestAlternate))
        Value = way_points.get_calc(Calculated().BestAlternate).GR;
      else
        Value=INVALID_GR;
#endif
    }
    break;
  default:
    Value=66.6; // something evil to notice..
    break;
  }

  Valid=false;
  if (Value < INVALID_GR) {
    Valid = true;
    if (Value >= 100 ) {
      _tcscpy(Format, _T("%1.0f"));
    } else {
      _tcscpy(Format, _T("%1.1f"));
    }
  }
}


const TCHAR *FormatterDiffBearing::Render(int *color) {

  if (Calculated().task_stats.task_valid
      && (Calculated().task_stats.current_leg.solution_remaining.Vector.Distance > 10.0)) {
    Valid = true;

    Value = Calculated().task_stats.current_leg.solution_remaining.Vector.Bearing 
      -  Basic().TrackBearing;

    // TODO use AngleLimit180
    if (Value < -180.0)
      Value += 360.0;
    else
    if (Value > 180.0)
      Value -= 360.0;

#ifndef __MINGW32__
    if (Value > 1)
      _stprintf(Text, _T("%2.0f°»"), Value);
    else if (Value < -1)
      _stprintf(Text, _T("«%2.0f°"), -Value);
    else
      _tcscpy(Text, _T("«»"));
#else
    if (Value > 1)
      _stprintf(Text, _T("%2.0fÂ°Â»"), Value);
    else if (Value < -1)
      _stprintf(Text, _T("Â«%2.0fÂ°"), -Value);
    else
      _tcscpy(Text, _T("Â«Â»"));
#endif
    *color = 0;
  } else {
    Valid = false;
    RenderInvalid(color);
  }
  return(Text);
}

