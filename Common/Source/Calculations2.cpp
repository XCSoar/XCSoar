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

#include "Calculations2.h"
#include "Calculations.h"
#include "GlideComputer.hpp"
#include "XCSoar.h"
#include "Dialogs.h"
#include "Protection.hpp"
#include "Settings.hpp"
#include "SettingsTask.hpp"
#include "SettingsComputer.hpp"
#include <math.h>
#include "InputEvents.h"
#include "TeamCodeCalculation.h"
#include "Math/Earth.hpp"
#include "PeriodClock.hpp"
#include "Math/Pressure.h"
#include "WayPoint.hpp"

#include <tchar.h>

#include "Math/NavFunctions.hpp" // used for team code

#include <stdio.h>


static PeriodClock last_team_code_update;
DWORD lastTeamCodeUpdateTime = GetTickCount();

void
CalculateOwnTeamCode(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (!WayPointList) return;
  if (TeamCodeRefWaypoint < 0) return;

  if (!last_team_code_update.check_update(10000))
    return;


  // JMW TODO: locking
  double distance = 0;
  double bearing = 0;
  TCHAR code[10];

  LL_to_BearRange(WayPointList[TeamCodeRefWaypoint].Latitude,
                  WayPointList[TeamCodeRefWaypoint].Longitude,
                  Basic->Latitude,
                  Basic->Longitude,
                  &bearing, &distance);

  GetTeamCode(code, bearing, distance);

  Calculated->TeammateBearing = bearing;
  Calculated->TeammateRange = distance;

  _tcsncpy(Calculated->OwnTeamCode, code, 5);
}

void
CalculateTeammateBearingRange(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  // JMW TODO: locking

  static bool InTeamSector = false;

  if (!WayPointList) return;
  if (TeamCodeRefWaypoint < 0) return;

  double ownDistance = 0;
  double ownBearing = 0;
  double mateDistance = 0;
  double mateBearing = 0;

  LL_to_BearRange(WayPointList[TeamCodeRefWaypoint].Latitude,
                  WayPointList[TeamCodeRefWaypoint].Longitude,
                  Basic->Latitude,
                  Basic->Longitude,
                  &ownBearing, &ownDistance);

  if (TeammateCodeValid)
    {

      CalcTeammateBearingRange(ownBearing, ownDistance,
                               TeammateCode,
                               &mateBearing, &mateDistance);

      // TODO code ....change the result of CalcTeammateBearingRange to do this !
      if (mateBearing > 180)
        {
          mateBearing -= 180;
        }
      else
        {
          mateBearing += 180;
        }


      Calculated->TeammateBearing = mateBearing;
      Calculated->TeammateRange = mateDistance;

      FindLatitudeLongitude(Basic->Latitude,
                            Basic->Longitude,
                            mateBearing,
                            mateDistance,
                            &TeammateLatitude,
                            &TeammateLongitude);

      if (mateDistance < 100 && InTeamSector==false)
        {
          InTeamSector=true;
          InputEvents::processGlideComputer(GCE_TEAM_POS_REACHED);
        }
      else if (mateDistance > 300)
        {
          InTeamSector = false;
        }
    }
  else
    {
      Calculated->TeammateBearing = 0;
      Calculated->TeammateRange = 0;
    }
}

