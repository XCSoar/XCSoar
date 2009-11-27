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

#include "Airspace.h"
#include "AirspaceDatabase.hpp"
#include "SettingsComputer.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

AirspaceDatabase airspace_database;

POINT *AirspaceScreenPoint;

/**
 * Deletes all airspaces in memory
 */
void DeleteAirspace() {
  if (AirspaceScreenPoint != NULL)  {
    LocalFree((HLOCAL)AirspaceScreenPoint);
    AirspaceScreenPoint = NULL;
  }

  airspace_database.Clear();
}

/**
 * Checks whether a longitude is between a certain
 * minimum and maximum
 * @param longitude Longitude to be checked
 * @param lon_min Minimum longitude
 * @param lon_max Maximum longitude
 * @return True if longitude is between the limits, False otherwise
 */
bool CheckInsideLongitude(double longitude,
                          const double lon_min, const double lon_max) {
  if (lon_min<=lon_max) {
    // normal case
    return ((longitude>lon_min) && (longitude<lon_max));
  } else {
    // area goes across 180 degree boundary, so lon_min is +ve, lon_max is -ve (flipped)
    return ((longitude>lon_min) || (longitude<lon_max));
  }
}

/* unused
int FindAirspaceCircle(const GEOPOINT &location, bool visibleonly)
{
  unsigned i;
  double basealt;
  double topalt;
 // int NearestIndex = 0;

  if(NumberOfAirspaceCircles == 0)
    {
      return -1;
    }

  for(i=0;i<NumberOfAirspaceCircles;i++) {
    if (iAirspaceMode[AirspaceCircle[i].Type]< 2) {
      // don't want warnings for this one
      continue;
    }
    if(AirspaceCircle[i].Visible || (!visibleonly)) {

      if (AirspaceCircle[i].Base.Base != abAGL) {
          basealt = AirspaceCircle[i].Base.Altitude;
        } else {
          basealt = AirspaceCircle[i].Base.AGL + XCSoarInterface::Calculated().TerrainAlt;
        }
      if (AirspaceCircle[i].Top.Base != abAGL) {
          topalt = AirspaceCircle[i].Top.Altitude;
        } else {
          topalt = AirspaceCircle[i].Top.AGL + XCSoarInterface::Calculated().TerrainAlt;
        }
      if(CheckAirspaceAltitude(basealt, topalt, settings)) {
        if (InsideAirspaceCircle(location,i)) {
	  return i;
        }
      }
    }
  }
  return -1;
}
*/

/**
 * Checks whether to display an airspace depending
 * on the AltitudeMode setting and the altitude
 * @param Base Airspace base altitude
 * @param Top Airspace top altitude
 * @param settings Pointer to the settings
 * @return True if airspace is supposed to be drawn, False otherwise
 */
bool
CheckAirspaceAltitude(double Base, double Top, double alt,
                      const SETTINGS_COMPUTER &settings)
{
  switch (settings.AltitudeMode) {
  case ALLON:
    return true;

  case CLIP:
    if (Base < settings.ClipAltitude)
      return true;
    else
      return false;

  case AUTO:
    if ((alt > (Base - settings.AltWarningMargin)) && (alt < (Top
        + settings.AltWarningMargin)))
      return true;
    else
      return false;

  case ALLBELOW:
    if ((Base - settings.AltWarningMargin) < alt)
      return true;
    else
      return false;

  case INSIDE:
    if ((alt >= (Base)) && (alt < (Top)))
      return true;
    else
      return false;

  case ALLOFF:
    return false;
  }

  return true;
}

/*
int FindAirspaceArea(double Longitude,double Latitude, bool visibleonly)
{
  unsigned i;
  double basealt;
  double topalt;

  if(NumberOfAirspaceAreas == 0)
    {
      return -1;
    }
  for(i=0;i<NumberOfAirspaceAreas;i++) {
    if (iAirspaceMode[AirspaceArea[i].Type]< 2) {
      // don't want warnings for this one
      continue;
    }
    if(AirspaceArea[i].Visible || (!visibleonly)) {

      if (AirspaceArea[i].Base.Base != abAGL) {
          basealt = AirspaceArea[i].Base.Altitude;
      } else {
          basealt = AirspaceArea[i].Base.AGL + XCSoarInterface::Calculated().TerrainAlt;
      }
      if (AirspaceArea[i].Top.Base != abAGL) {
          topalt = AirspaceArea[i].Top.Altitude;
      } else {
          topalt = AirspaceArea[i].Top.AGL + XCSoarInterface::Calculated().TerrainAlt;
      }
      if(CheckAirspaceAltitude(basealt, topalt)) {
        if (InsideAirspaceArea(Longitude,Latitude,i)) {
          return i;
        }
      }
    }
  }
  // not inside any airspace
  return -1;
}
*/

// Finds nearest airspace (whether circle or area) to the specified point. Returns -1 in foundcircle or foundarea if circle or area is not found. Otherwise, returns index of the circle or area that is closest to the specified point. Also returns the distance and bearing to the boundary of the airspace. Distance < 0 means interior. This only searches within a range of 100km of the target
/**
 * Finds nearest airspace (whether circle or area)
 * to the specified point.
 *
 * Returns -1 in foundcircle or foundarea if circle or
 * area is not found. Otherwise, returns index of the
 * circle or area that is closest to the specified point.
 *
 * Also returns the distance and bearing to the boundary
 * of the airspace.
 * (Distance < 0 means interior)
 *
 * This only searches within a range of 100km of the target.
 * @param location Location where to search
 * @param settings Settings object
 * @param map_projection MapWindowProjection object
 * @param nearestdistance Distance to nearest airspace (Pointer)
 * @param nearestbearing Bearing to nearest airspace (Pointer)
 * @param foundcircle Array id of nearest AirspaceCircle
 * @param foundarea Array if of nearest AirspaceArea
 * @param height If != NULL only airspaces are considered
 * where the height is between base and top of the airspace
 */
void FindNearestAirspace(const GEOPOINT &location,
                         double altitude, double terrain_altitude,
    const SETTINGS_COMPUTER &settings,
    const MapWindowProjection& map_projection, double *nearestdistance,
    double *nearestbearing, int *foundcircle, int *foundarea, double *height) {

  // TODO enhancement: return also the vertical separation
  double nearestd1 = 100000; // 100km
  double nearestd2 = 100000; // 100km
  double nearestb1 = 0;
  double nearestb2 = 0;

  *foundcircle = airspace_database.NearestCircle(location, altitude,
                                                 terrain_altitude, settings,
                                                 &nearestd1, &nearestb1,
                                                 height);

  *foundarea = airspace_database.NearestArea(location, altitude,
                                             terrain_altitude, settings,
                                             map_projection,
                                             &nearestd2, &nearestb2, height);

  if ((*foundcircle >= 0) && (*foundarea < 0)) {
    *nearestdistance = nearestd1;
    *nearestbearing = nearestb1;
    *foundarea = -1;
    return;
  }
  if ((*foundarea >= 0) && (*foundcircle < 0)) {
    *nearestdistance = nearestd2;
    *nearestbearing = nearestb2;
    *foundcircle = -1;
    return;
  }

  if (nearestd1 < nearestd2) {
    if (nearestd1 < 100000) {
      *nearestdistance = nearestd1;
      *nearestbearing = nearestb1;
      *foundarea = -1;
    }
  } else {
    if (nearestd2 < 100000) {
      *nearestdistance = nearestd2;
      *nearestbearing = nearestb2;
      *foundcircle = -1;
    }
  }

  return;
}
