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
#include "UtilsAirspace.hpp"
#include "Utils.h"
#include "XCSoar.h"
#include "LogFile.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "Settings.hpp"
#include "Units.h"
#include "Math/Pressure.h"
#include "Math/Units.h"
#include <assert.h>


void ConvertFlightLevels(void)
{
  unsigned i;

  // TODO accuracy: Convert flightlevels is inaccurate!

  for(i=0;i<NumberOfAirspaceCircles;i++)
    {
      if(AirspaceCircle[i].Base.FL  != 0)
	{
	  AirspaceCircle[i].Base.Altitude = (AirspaceCircle[i].Base.FL * 100) + ((QNH-1013)*30);
	  AirspaceCircle[i].Base.Altitude = AirspaceCircle[i].Base.Altitude / TOFEET;
	}
      if(AirspaceCircle[i].Top.FL  != 0)
	{
	  AirspaceCircle[i].Top.Altitude = (AirspaceCircle[i].Top.FL * 100) + ((QNH-1013)*30);
	  AirspaceCircle[i].Top.Altitude = AirspaceCircle[i].Top.Altitude / TOFEET;
	}
    }


  for(i=0;i<NumberOfAirspaceAreas;i++)
    {
      if(AirspaceArea[i].Base.FL  != 0)
	{
	  AirspaceArea[i].Base.Altitude = (AirspaceArea[i].Base.FL * 100) + ((QNH-1013)*30);
	  AirspaceArea[i].Base.Altitude = AirspaceArea[i].Base.Altitude / TOFEET;
	}
      if(AirspaceArea[i].Top.FL  != 0)
	{
	  AirspaceArea[i].Top.Altitude = (AirspaceArea[i].Top.FL * 100) + ((QNH-1013)*30);
	  AirspaceArea[i].Top.Altitude = AirspaceArea[i].Top.Altitude / TOFEET;
	}
    }
}


void FormatWarningString(int Type, const TCHAR *Name,
                         AIRSPACE_ALT Base, AIRSPACE_ALT Top,
                         TCHAR *szMessageBuffer, TCHAR *szTitleBuffer)
{
  TCHAR BaseStr[512];
  TCHAR TopStr[512];

  switch (Type)
    {
    case RESTRICT:
      _tcscpy(szTitleBuffer,gettext(TEXT("Restricted"))); break;
    case PROHIBITED:
      _tcscpy(szTitleBuffer,gettext(TEXT("Prohibited"))); break;
    case DANGER:
      _tcscpy(szTitleBuffer,gettext(TEXT("Danger Area"))); break;
    case CLASSA:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class A"))); break;
    case CLASSB:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class B"))); break;
    case CLASSC:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class C"))); break;
    case CLASSD:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class D"))); break;
    case CLASSE:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class E"))); break;
    case CLASSF:
      _tcscpy(szTitleBuffer,gettext(TEXT("Class F"))); break;
    case NOGLIDER:
      _tcscpy(szTitleBuffer,gettext(TEXT("No Glider"))); break;
    case CTR:
      _tcscpy(szTitleBuffer,gettext(TEXT("CTR"))); break;
    case WAVE:
      _tcscpy(szTitleBuffer,gettext(TEXT("Wave"))); break;
    default:
      _tcscpy(szTitleBuffer,gettext(TEXT("Unknown")));
    }

  if(Base.FL == 0)
    {
      if (Base.AGL > 0) {
        _stprintf(BaseStr,TEXT("%1.0f%s %s"),
                  ALTITUDEMODIFY * Base.AGL,
                  Units::GetUnitName(Units::GetUserAltitudeUnit()),
                  gettext(TEXT("AGL")));
      } else if (Base.Altitude > 0)
        _stprintf(BaseStr,TEXT("%1.0f%s %s"),
                  ALTITUDEMODIFY * Base.Altitude,
                  Units::GetUnitName(Units::GetUserAltitudeUnit()),
                  gettext(TEXT("MSL")));
      else
        _stprintf(BaseStr,gettext(TEXT("GND")));
    }
  else
    {
      _stprintf(BaseStr,TEXT("FL %1.0f"),Base.FL );
    }

  if(Top.FL == 0)
    {
      if (Top.AGL > 0) {
        _stprintf(TopStr,TEXT("%1.0f%s %s"),
                  ALTITUDEMODIFY * Top.AGL,
                  Units::GetUnitName(Units::GetUserAltitudeUnit()),
                  gettext(TEXT("AGL")));
      } else {
	_stprintf(TopStr,TEXT("%1.0f%s %s"), ALTITUDEMODIFY * Top.Altitude,
		  Units::GetUnitName(Units::GetUserAltitudeUnit()),
		  gettext(TEXT("MSL")));
      }
    }
  else
    {
      _stprintf(TopStr,TEXT("FL %1.0f"),Top.FL );
    }

  _stprintf(szMessageBuffer,TEXT("%s: %s\r\n%s: %s\r\n%s: %s\r\n"),
            szTitleBuffer,
            Name,
            gettext(TEXT("Top")),
            TopStr,
            gettext(TEXT("Base")),
            BaseStr
            );
}
