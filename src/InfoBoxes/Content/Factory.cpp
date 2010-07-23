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

#include "InfoBoxes/Content/Factory.hpp"

#include "InfoBoxes/Content/Base.hpp"
#include "InfoBoxes/Content/Alternate.hpp"
#include "InfoBoxes/Content/Altitude.hpp"
#include "InfoBoxes/Content/Direction.hpp"
#include "InfoBoxes/Content/Speed.hpp"
#include "InfoBoxes/Content/Task.hpp"
#include "InfoBoxes/Content/Team.hpp"
#include "InfoBoxes/Content/Thermal.hpp"
#include "InfoBoxes/Content/Time.hpp"
#include "InfoBoxes/Content/Weather.hpp"

#include <stddef.h>

InfoBoxContent*
InfoBoxFactory::Create(unsigned InfoBoxType)
{
  switch (InfoBoxType) {
  case 0:
    return new InfoBoxContentAltitudeGPS();
  case 1:
    return new InfoBoxContentAltitudeAGL();
  case 2:
    return new InfoBoxContentThermal30s();
  case 3:
    return new InfoBoxContentBearing();
  case 6:
    return new InfoBoxContentSpeedGround();
  case 7:
    return new InfoBoxContentThermalLastAvg();
  case 8:
    return new InfoBoxContentThermalLastGain();
  case 9:
    return new InfoBoxContentThermalLastTime();
  case 10:
    return new InfoBoxContentMacCready();
  case 11:
    return new InfoBoxContentNextDistance();
  case 12:
    return new InfoBoxContentNextAltitudeDiff();
  case 13:
    return new InfoBoxContentNextAltitudeRequire();
  case 14:
    return new InfoBoxContentNextWaypoint();
  case 15:
    return new InfoBoxContentFinalAltitudeDiff();
  case 16:
    return new InfoBoxContentFinalAltitudeRequire();
  case 17:
    return new InfoBoxContentTaskSpeed();
  case 18:
    return new InfoBoxContentFinalDistance();
  case 19:
    return new InfoBoxContentFinalLD();
  case 20:
    return new InfoBoxContentTerrainHeight();
  case 21:
    return new InfoBoxContentThermalAvg();
  case 22:
    return new InfoBoxContentThermalGain();
  case 23:
    return new InfoBoxContentTrack();
  case 24:
    return new InfoBoxContentVario();
  case 25:
    return new InfoBoxContentWindSpeed();
  case 26:
    return new InfoBoxContentWindBearing();
  case 27:
    return new InfoBoxContentTaskAATime();
  case 28:
    return new InfoBoxContentTaskAADistanceMax();
  case 29:
    return new InfoBoxContentTaskAADistanceMin();
  case 30:
    return new InfoBoxContentTaskAASpeedMax();
  case 31:
    return new InfoBoxContentTaskAASpeedMin();
  case 32:
    return new InfoBoxContentSpeedIndicated();
  case 33:
    return new InfoBoxContentAltitudeBaro();
  case 34:
    return new InfoBoxContentSpeedMacCready();
  case 35:
    return new InfoBoxContentThermalRatio();
  case 36:
    return new InfoBoxContentTimeFlight();
  case 39:
    return new InfoBoxContentTimeLocal();
  case 40:
    return new InfoBoxContentTimeUTC();
  case 41:
    return new InfoBoxContentFinalETE();
  case 42:
    return new InfoBoxContentNextETE();
  case 43:
    return new InfoBoxContentSpeedDolphin();
  case 44:
    return new InfoBoxContentVarioNetto();
  case 45:
    return new InfoBoxContentFinalETA();
  case 46:
    return new InfoBoxContentNextETA();
  case 47:
    return new InfoBoxContentBearingDiff();
  case 48:
    return new InfoBoxContentTemperature();
  case 49:
    return new InfoBoxContentHumidity();
  case 50:
    return new InfoBoxContentTemperatureForecast();
  case 51:
    return new InfoBoxContentTaskAADistance();
  case 52:
    return new InfoBoxContentTaskAASpeed();
  case 54:
    return new InfoBoxContentSpeed();
  case 55:
    return new InfoBoxContentTeamCode();
  case 56:
    return new InfoBoxContentTeamBearing();
  case 57:
    return new InfoBoxContentTeamBearingDiff();
  case 58:
    return new InfoBoxContentTeamDistance();
  case 61:
    return new InfoBoxContentTaskSpeedAchieved();
  case 62:
    return new InfoBoxContentTaskAATimeDelta();
  case 63:
    return new InfoBoxContentThermalAllAvg();
  case 67:
    return new InfoBoxContentAlternate1();
  case 68:
    return new InfoBoxContentAlternate2();
  case 69:
    return new InfoBoxContentAlternateBest();
  case 70:
    return new InfoBoxContentAltitudeQFE();
  }

  return NULL;
}
