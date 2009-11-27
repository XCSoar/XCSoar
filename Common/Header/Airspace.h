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

#if !defined(XCSOAR_AIRSPACE_H)
#define XCSOAR_AIRSPACE_H

#include "Sizes.h"
#include "Screen/shapelib/mapshape.h"
#include "GeoPoint.hpp"

#include <windef.h>
#include <tchar.h>

struct SETTINGS_COMPUTER;
class AirspaceDatabase;
class MapWindowProjection;

struct AIRSPACE_ACK
{
  bool AcknowledgedToday;
  double AcknowledgementTime;
};

typedef enum {abUndef, abMSL, abAGL, abFL} AirspaceAltBase_t;

struct AIRSPACE_ALT
{
  double Altitude;
  double FL;
  double AGL;
  AirspaceAltBase_t Base;
};

struct AirspaceMetadata {
  TCHAR Name[NAME_SIZE + 1];
  int Type;
  AIRSPACE_ALT Base;
  AIRSPACE_ALT Top;

  rectObj bounds;
  AIRSPACE_ACK Ack;
  unsigned char WarningLevel; // 0= no warning, 1= predicted incursion, 2= entered
  bool FarVisible;
};

struct AIRSPACE_AREA : public AirspaceMetadata
{
  unsigned FirstPoint;
  unsigned NumPoints;
  unsigned char Visible;
  bool _NewWarnAckNoBrush;
  GEOPOINT minBound;
  GEOPOINT maxBound;
};

#define AIRSPACE_POINT GEOPOINT
// quick hack...

struct AIRSPACE_CIRCLE : public AirspaceMetadata
{
  GEOPOINT Location;
  double Radius;
  POINT Screen;
  int ScreenR;
  unsigned char Visible;
  bool _NewWarnAckNoBrush;
};

extern POINT *AirspaceScreenPoint;

static inline double
ToMSL(const AIRSPACE_ALT &altitude, double terrain_altitude)
{
  return altitude.Base != abAGL
    ? altitude.Altitude
    : altitude.AGL + terrain_altitude;
}

void DeleteAirspace(AirspaceDatabase &airspace_database);

void ReadAirspace(AirspaceDatabase &airspace_database);

bool
CheckAirspaceAltitude(double Base, double Top, double altitude,
                      const SETTINGS_COMPUTER &settings);

void CloseAirspace(AirspaceDatabase &airspace_database);

void SortAirspace(AirspaceDatabase &airspace_database);

void
FindNearestAirspace(AirspaceDatabase &airspace_database,
                    const GEOPOINT &location,
                    double altitude, double terrain_altitude,
                    const SETTINGS_COMPUTER &settings,
                    const MapWindowProjection &map_projection,
                    double *nearestdistance, double *nearestbearing,
                    int *foundcircle,
                    int *foundarea,
                    double *height=NULL);

#endif
