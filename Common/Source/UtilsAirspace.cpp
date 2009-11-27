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

#include "UtilsAirspace.hpp"
#include "AirspaceDatabase.hpp"
#include "SettingsAirspace.hpp"
#include "LogFile.hpp"
#include "Dialogs.h"
#include "Language.hpp"
#include "Units.hpp"
#include "Math/Pressure.h"
#include "Math/Units.h"
#include <assert.h>
#include "Units.hpp"

static void
ConvertFlightLevels(AIRSPACE_ALT &altitude)
{
    if (altitude.FL != 0) {
      altitude.Altitude = altitude.FL * 100 + (QNH - 1013) * 30;
      altitude.Altitude = altitude.Altitude / TOFEET;
    }
}

static void
ConvertFlightLevels(AirspaceMetadata &airspace)
{
  ConvertFlightLevels(airspace.Base);
  ConvertFlightLevels(airspace.Top);
}

/**
 * Converts all FlightLevel-based airspaces to MSL-based airspaces
 * (Attention: Inaccurate!)
 */
void
ConvertFlightLevels(AirspaceDatabase &airspace_database)
{
  // TODO accuracy: ConvertFlightLevels is inaccurate!

  for (unsigned i = 0; i < airspace_database.NumberOfAirspaceCircles; ++i)
    ConvertFlightLevels(airspace_database.AirspaceCircle[i]);

  for (unsigned i = 0; i < airspace_database.NumberOfAirspaceAreas; ++i)
    ConvertFlightLevels(airspace_database.AirspaceArea[i]);
}

/**
 * Fills the buffers with a formatted airspace warning
 * string
 * @param Type Airspace type
 * @param Name Airspace name
 * @param Base Airspace base
 * @param Top Airspace top
 * @param szMessageBuffer Pointer to the message buffer
 * @param szTitleBuffer Pointer to the title buffer
 */
void FormatWarningString(int Type, const TCHAR *Name, AIRSPACE_ALT Base,
    AIRSPACE_ALT Top, TCHAR *szMessageBuffer, TCHAR *szTitleBuffer) {
  TCHAR BaseStr[512];
  TCHAR TopStr[512];

  switch (Type) {
  case RESTRICT:
    _tcscpy(szTitleBuffer, gettext(TEXT("Restricted")));
    break;
  case PROHIBITED:
    _tcscpy(szTitleBuffer, gettext(TEXT("Prohibited")));
    break;
  case DANGER:
    _tcscpy(szTitleBuffer, gettext(TEXT("Danger Area")));
    break;
  case CLASSA:
    _tcscpy(szTitleBuffer, gettext(TEXT("Class A")));
    break;
  case CLASSB:
    _tcscpy(szTitleBuffer, gettext(TEXT("Class B")));
    break;
  case CLASSC:
    _tcscpy(szTitleBuffer, gettext(TEXT("Class C")));
    break;
  case CLASSD:
    _tcscpy(szTitleBuffer, gettext(TEXT("Class D")));
    break;
  case CLASSE:
    _tcscpy(szTitleBuffer, gettext(TEXT("Class E")));
    break;
  case CLASSF:
    _tcscpy(szTitleBuffer, gettext(TEXT("Class F")));
    break;
  case NOGLIDER:
    _tcscpy(szTitleBuffer, gettext(TEXT("No Glider")));
    break;
  case CTR:
    _tcscpy(szTitleBuffer, gettext(TEXT("CTR")));
    break;
  case WAVE:
    _tcscpy(szTitleBuffer, gettext(TEXT("Wave")));
    break;
  default:
    _tcscpy(szTitleBuffer, gettext(TEXT("Unknown")));
  }

  if (Base.FL == 0) {
    if (Base.AGL > 0) {
      _stprintf(BaseStr, TEXT("%1.0f%s %s"), ALTITUDEMODIFY * Base.AGL,
          Units::GetUnitName(Units::GetUserAltitudeUnit()),
          gettext(TEXT("AGL")));
    } else if (Base.Altitude > 0)
      _stprintf(BaseStr, TEXT("%1.0f%s %s"), ALTITUDEMODIFY * Base.Altitude,
          Units::GetUnitName(Units::GetUserAltitudeUnit()),
          gettext(TEXT("MSL")));
    else
      _stprintf(BaseStr, gettext(TEXT("GND")));
  } else {
    _stprintf(BaseStr, TEXT("FL %1.0f"), Base.FL);
  }

  if (Top.FL == 0) {
    if (Top.AGL > 0) {
      _stprintf(TopStr, TEXT("%1.0f%s %s"), ALTITUDEMODIFY * Top.AGL,
          Units::GetUnitName(Units::GetUserAltitudeUnit()),
          gettext(TEXT("AGL")));
    } else {
      _stprintf(TopStr, TEXT("%1.0f%s %s"), ALTITUDEMODIFY * Top.Altitude,
          Units::GetUnitName(Units::GetUserAltitudeUnit()),
          gettext(TEXT("MSL")));
    }
  } else {
    _stprintf(TopStr, TEXT("FL %1.0f"), Top.FL);
  }

  _stprintf(szMessageBuffer, TEXT("%s: %s\r\n%s: %s\r\n%s: %s\r\n"),
      szTitleBuffer, Name, gettext(TEXT("Top")), TopStr, gettext(TEXT("Base")),
      BaseStr);
}
