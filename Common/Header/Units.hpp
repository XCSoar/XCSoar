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

#if !defined(__UNITS_H)
#define __UNITS_H

#include <tchar.h>

#ifndef __GNUC__
#define DEG "°"
#else
#define DEG "Â°"
#endif

// units
extern double SPEEDMODIFY;
extern double LIFTMODIFY;
extern double DISTANCEMODIFY;
extern double ALTITUDEMODIFY;
extern double TASKSPEEDMODIFY;


typedef enum {
  cfDDMMSS=0,
  cfDDMMSSss,
  cfDDMMmmm,
  cfDDdddd
}CoordinateFormats_t;

typedef enum {
  unUndef,
  unKiloMeter,
  unNauticalMiles,
  unStatuteMiles,
  unKiloMeterPerHour,
  unKnots,
  unStatuteMilesPerHour,
  unMeterPerSecond,
  unFeetPerMinutes,
  unMeter,
  unFeet,
  unFligthLevel,
  unKelvin,
  unGradCelcius,                    // K = C° + 273,15
  unGradFahrenheit, // K = (°F + 459,67) / 1,8

  /**
   * The sentinel: the number of units in this enum.
   */
  unCount
}Units_t;


typedef enum {
  ugNone,
  ugDistance,
  ugAltitude,
  ugHorizontalSpeed,
  ugVerticalSpeed,
  ugWindSpeed,
  ugTaskSpeed
} UnitGroup_t;

typedef struct{
  const TCHAR   *Name;
  double  ToUserFact;
  double  ToUserOffset;
}UnitDescriptor_t;

class Units {

private:
  static UnitDescriptor_t UnitDescriptors[unCount];
  static Units_t UserDistanceUnit;
  static Units_t UserAltitudeUnit;
  static Units_t UserHorizontalSpeedUnit;
  static Units_t UserVerticalSpeedUnit;
  static Units_t UserWindSpeedUnit;
  static Units_t UserTaskSpeedUnit;

public:

  static CoordinateFormats_t CoordinateFormat;

  static const TCHAR *GetUnitName(Units_t Unit);

  static Units_t GetUserDistanceUnit(void);
  static Units_t SetUserDistanceUnit(Units_t NewUnit);

  static Units_t GetUserAltitudeUnit(void);
  static Units_t SetUserAltitudeUnit(Units_t NewUnit);

  static Units_t GetUserHorizontalSpeedUnit(void);
  static Units_t SetUserHorizontalSpeedUnit(Units_t NewUnit);

  static Units_t GetUserTaskSpeedUnit(void);
  static Units_t SetUserTaskSpeedUnit(Units_t NewUnit);

  static Units_t GetUserVerticalSpeedUnit(void);
  static Units_t SetUserVerticalSpeedUnit(Units_t NewUnit);

  static Units_t GetUserWindSpeedUnit(void);
  static Units_t SetUserWindSpeedUnit(Units_t NewUnit);

  static Units_t GetUserUnitByGroup(UnitGroup_t UnitGroup);

  static void LongitudeToDMS(double Longitude,
                             int *dd,
                             int *mm,
                             int *ss,
                             bool *east);
  static void LatitudeToDMS(double Latitude,
                            int *dd,
                            int *mm,
                            int *ss,
                            bool *north);

  static bool LongitudeToString(double Longitude, TCHAR *Buffer, size_t size);
  static bool LatitudeToString(double Latitude, TCHAR *Buffer, size_t size);

  static void NotifyUnitChanged(void);

  static const TCHAR *GetHorizontalSpeedName();

  static const TCHAR *GetVerticalSpeedName();

  static const TCHAR *GetDistanceName();

  static const TCHAR *GetAltitudeName();

  static const TCHAR *GetTaskSpeedName();

  static bool FormatUserAltitude(double Altitude, TCHAR *Buffer, size_t size);
  static bool FormatAlternateUserAltitude(double Altitude, TCHAR *Buffer, size_t size);
  static bool FormatUserArrival(double Altitude, TCHAR *Buffer, size_t size); // VENTA3
  static bool FormatUserDistance(double Distance, TCHAR *Buffer, size_t size);
  static bool FormatUserMapScale(Units_t *Unit, double Distance, TCHAR *Buffer, size_t size);

  static double ToUserAltitude(double Altitude);
  static double ToSysAltitude(double Altitude);

  static double ToUserDistance(double Distance);
  static double ToSysDistance(double Distance);

  static void TimeToText(TCHAR* text, int d);

};

#endif

