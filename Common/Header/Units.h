/*
  XCSoar Glide Computer
  Copyright (C) 2005  ...

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

*/



typedef enum {
  cfDDMMSS,
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
  unGradFahrenheit                  // K = (°F + 459,67) / 1,8
}Units_t;

typedef struct{
  TCHAR  *Name;
  double ToUserFact;
  double ToUserOffset;
}UnitDescriptor_t;

class Units {

private:
  static UnitDescriptor_t UnitDescriptors[unGradFahrenheit+1];
  static Units_t UserDistanceUnit;
  static Units_t UserAltitudeUnit;
  static Units_t UserHorizontalSpeedUnit;
  static Units_t UserVerticalSpeedUnit;
  static Units_t UserWindSpeedUnit;

public:

  static CoordinateFormats_t CoordinateFormat;

  static TCHAR *GetUnitName(Units_t Unit);

  static Units_t GetUserDistanceUnit(void);
  static Units_t SetUserDistanceUnit(Units_t NewUnit);

  static Units_t GetUserAltitudeUnit(void);
  static Units_t SetUserAltitudeUnit(Units_t NewUnit);

  static Units_t GetUserHorizontalSpeedUnit(void);
  static Units_t SetUserHorizontalSpeedUnit(Units_t NewUnit);

  static Units_t GetUserVerticalSpeedUnit(void);
  static Units_t SetUserVerticalSpeedUnit(Units_t NewUnit);

  static Units_t GetUserWindSpeedUnit(void);
  static Units_t SetUserWindSpeedUnit(Units_t NewUnit);

  static bool LongitudeToString(double Longitude, TCHAR *Buffer, size_t size);
  static bool LatitudeToString(double Latitude, TCHAR *Buffer, size_t size);

  static void NotifyUnitChanged(void);

  static TCHAR *GetHorizontalSpeedName();

  static TCHAR *GetVerticalSpeedName();

  static TCHAR *GetDistanceName();

  static TCHAR *GetAltitudeName();

  static bool FormatUserAltitude(double Altitude, TCHAR *Buffer, size_t size);
  static bool FormatUserDistance(double Distance, TCHAR *Buffer, size_t size);

};
