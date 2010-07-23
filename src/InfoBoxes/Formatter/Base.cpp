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

#include "InfoBoxes/Formatter/Base.hpp"
#include "Math/FastMath.h"
#include "Atmosphere.h"
#include "Hardware/Battery.h"
#include "Units.hpp"
#include "Interface.hpp"
#include <stdio.h>
#include "Components.hpp"
#include "Asset.hpp"

InfoBoxFormatter::InfoBoxFormatter(const TCHAR *theformat)
{
  _tcscpy(Format, theformat);
  Valid = true;
  Value = 0.0;
  Text[0] = 0;
  CommentText[0] = 0;
}

const TCHAR *
InfoBoxFormatter::Render(int *color)
{
  if (Valid) {
    _stprintf(Text, Format, Value);
    *color = 0;
  } else {
    RenderInvalid(color);
  }
  return Text;
}

const TCHAR *
InfoBoxFormatter::RenderTitle(int *color)
{
  if (Valid) {
    _stprintf(Text, Format, Value);
    *color = 0;
  } else {
    RenderInvalid(color);
  }
  return Text;
}

void
InfoBoxFormatter::RenderInvalid(int *color)
{
  _tcscpy(CommentText, _T(""));
  _tcscpy(Text, _T("---"));
  *color = -1;
}

// TODO enhancement: crop long text or provide alternate
// e.g. 10300 ft ==> 10,3
// e.g. "Ardlethan" => "Ardl."

void
InfoBoxFormatter::AssignValue(int i)
{
  switch (i) {
  case 4:
    if (Calculated().LD == fixed(999)) {
      Valid = false;
    } else {
      Valid = true;
      Value = Calculated().LD;
    }
    break;

  case 5:
    if (Calculated().CruiseLD == fixed(999)) {
      Valid = false;
    } else {
      Valid = true;
      Value = Calculated().CruiseLD;
    }
    break;

  case 37:
    Valid = Basic().acceleration.Available;
    Value = Basic().acceleration.Gload;
    break;

  case 38:
#ifdef OLD_TASK
    if (Calculated().LDNext== 999) {
      Valid = false;
    } else {
      Valid = Calculated().task_stats.task_valid;
      Value = Calculated().LDNext;
    }
#endif
    break;

  case 53:
    if (Calculated().LDvario == fixed(999)) {
      Valid = false;
    } else {
      Valid = Basic().TotalEnergyVarioAvailable && Basic().AirspeedAvailable;
      Value = Calculated().LDvario;
    }
    break;

  case 65: // battery voltage
#if !defined(WINDOWSPC) && !defined(HAVE_POSIX)
    if (!is_altair()) {
      Value = PDABatteryPercent;
      Valid = true;
    } else {
      Value = Basic().SupplyBatteryVoltage;
      if (Value > 0.0)
        Valid = true;
      else
        Valid = false;
    }
#else
    Value = 0.0;
    Valid = false;
#endif
    break;

  case 66: // GR Final
#ifdef OLD_TASK
    Valid = Calculated().task_stats.task_valid;
    if (Calculated().GRFinish== 999) {
      Valid = false;
    } else {
      Valid = Calculated().task_stats.task_valid;
      if (Calculated().ValidFinish) {
        Value = 0;
      } else {
        Value = Calculated().GRFinish;
        if (Value > 100)
          _tcscpy(Format, _T("%1.0f"));
        else
          _tcscpy(Format, _T("%1.1f"));
      }
    }
#endif      
    break;

  case 71:
    if (Calculated().AverageLD == 0) {
      Valid = false;
    } else {
      Valid = true;
      Value = Calculated().AverageLD;
      if (Value < 0)
        _tcscpy(Format, _T("^^^"));
      else if (Value >= 999)
        _tcscpy(Format, _T("+++"));
      else
        _tcscpy(Format, _T("%2.0f"));
    }
    break;

  case 72:
    Value = Calculated().Experimental;
    Valid = true;
    break;

  /* TODO feature: add extra infoboxes from Lars
  case 68: // distance flown
    if (Calculated().TaskDistanceCovered != 0) {
      Value = Units::ToUserDistance(Calculated().TaskDistanceCovered);
      Valid = true;
    } else {
      Value = 0.0;
      Valid = false;
    }
    break;

  case 67: // termik liga points
    if (Calculated().TermikLigaPoints != 0) {
      Value = Calculated().TermikLigaPoints;
      Valid = true;
    } else {
      Value = 0.0;
      Valid = false;
    }
    break;
    */

  default:
    break;
  };
}

const TCHAR *
InfoBoxFormatter::GetCommentText(void)
{
  return CommentText;
}

bool
InfoBoxFormatter::isValid(void)
{
  return Valid;
}
