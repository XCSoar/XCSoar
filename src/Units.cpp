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

// TODO code: units stuff
// - check buffer size in LongitudeToString and LatiditudeToString
// - unit dialog support

//default       EU   UK   US   AUS
//altitude      m    ft   ft   m
//verticalspeed m/s  kts  kts  kts
//wind speed    km/  kts  mp   kts
//IAS           km/  kts  mp   kts
//distance      km   nm   ml   nm

#include "Units.hpp"
#include "Math/Angle.hpp"

#include <stdio.h>

#include "resource.h"
#include "SettingsMap.hpp"

#include <stdlib.h>
#include <math.h>
#include <tchar.h>

CoordinateFormats_t Units::CoordinateFormat;

//SI to Local Units

const UnitDescriptor_t Units::UnitDescriptors[] = {
  { NULL, fixed_one, fixed_zero },
  { _T("km"), fixed_constant(0.001, 0x41893LL), fixed_zero },
  { _T("nm"), fixed_constant(0.000539956803, 0x2362fLL), fixed_zero },
  { _T("sm"), fixed_constant(0.000621371192, 0x28b8eLL), fixed_zero },
  { _T("km/h"), fixed_constant(3.6, 0x39999999LL), fixed_zero },
  { _T("kt"), fixed_constant(1.94384449, 0x1f19fcaeLL), fixed_zero },
  { _T("mph"), fixed_constant(2.23693629, 0x23ca7db5LL), fixed_zero },
  { _T("m/s"), fixed_one, fixed_zero },
  { _T("fpm"), fixed_constant(196.850394, 0xc4d9b36bdLL), fixed_zero },
  { _T("m"), fixed_one, fixed_zero },
  { _T("ft"), fixed_constant(3.2808399, 0x347e51faLL), fixed_zero },
  { _T("FL"), fixed_constant(0.032808399, 0x866219LL), fixed_zero },
  { _T("K"), fixed_one, fixed_zero },
  { _T(DEG)_T("C"), fixed_one, fixed_constant(-273.15, -73323144806LL) },
  { _T(DEG)_T("F"), fixed_constant(1.8, 0x1cccccccLL),
    fixed_constant(-459.67, -123391726059LL) }
};

Units_t Units::DistanceUnit = unKiloMeter;
Units_t Units::AltitudeUnit = unMeter;
Units_t Units::TemperatureUnit = unGradCelcius;
Units_t Units::SpeedUnit = unKiloMeterPerHour;
Units_t Units::VerticalSpeedUnit = unMeterPerSecond;
Units_t Units::WindSpeedUnit = unKiloMeterPerHour;
Units_t Units::TaskSpeedUnit = unKiloMeterPerHour;

void
Units::LongitudeToDMS(Angle Longitude, int *dd, int *mm, int *ss, bool *east)
{
  // if (Longitude is negative) -> Longitude is West otherwise East
  *east = (Longitude.sign() < 0 ? false : true);

  unsigned value = (unsigned)(Longitude.magnitude_degrees() * 3600 +
                              fixed_half);

  *ss = value % 60;
  value /= 60;

  *mm = value % 60;
  value /= 60;

  *dd = value;
}

void
Units::LatitudeToDMS(Angle Latitude, int *dd, int *mm, int *ss, bool *north)
{
  // if (Latitude is negative) -> Latitude is South otherwise North
  *north = (Latitude.sign() < 0 ? false : true);

  unsigned value = (unsigned)(Latitude.magnitude_degrees() * 3600 +
                              fixed_half);

  *ss = value % 60;
  value /= 60;

  *mm = value % 60;
  value /= 60;

  *dd = value;
}

bool
Units::LongitudeToString(Angle Longitude, TCHAR *Buffer, size_t size)
{
  (void)size;
  TCHAR EW[] = _T("WEE");
  int dd, mm, ss;

  // Calculate Longitude sign
  int sign = Longitude.sign()+1;
  double mlong = Longitude.magnitude_degrees();

  switch (CoordinateFormat) {
  case cfDDMMSS:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    mm = (int)(mlong);
    // Calculate seconds
    mlong = (mlong - mm) * 60.0;
    ss = (int)(mlong + 0.5);
    if (ss >= 60) {
      mm++;
      ss -= 60;
    }
    if (mm >= 60) {
      dd++;
      mm -= 60;
    }
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%03d")_T(DEG)_T("%02d'%02d\""), EW[sign],
              dd, mm, ss);
    break;

  case cfDDMMSSss:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    mm = (int)(mlong);
    // Calculate seconds
    mlong = (mlong - mm) * 60.0;
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%03d")_T(DEG)_T("%02d'%05.2f\""), EW[sign],
              dd, mm, mlong);
    break;

  case cfDDMMmmm:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%03d")_T(DEG)_T("%06.3f'"), EW[sign], dd, mlong);
    break;

  case cfDDdddd:
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%08.4f")_T(DEG), EW[sign], mlong);
    break;

  default:
    return false;
  }

  return true;
}

bool
Units::LatitudeToString(Angle Latitude, TCHAR *Buffer, size_t size)
{
  (void)size;
  TCHAR EW[] = _T("SNN");
  int dd, mm, ss;

  // Calculate Latitude sign
  int sign = Latitude.sign()+1;
  double mlat = Latitude.magnitude_degrees();

  switch (CoordinateFormat) {
  case cfDDMMSS:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    mm = (int)(mlat);
    // Calculate seconds
    mlat = (mlat - mm) * 60.0;
    ss = (int)(mlat + 0.5);
    if (ss >= 60) {
      mm++;
      ss -= 60;
    }
    if (mm >= 60) {
      dd++;
      mm -= 60;
    }
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%02d")_T(DEG)_T("%02d'%02d\""), EW[sign],
              dd, mm, ss);
    break;

  case cfDDMMSSss:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    mm = (int)(mlat);
    // Calculate seconds
    mlat = (mlat - mm) * 60.0;
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%02d")_T(DEG)_T("%02d'%05.2f\""), EW[sign],
              dd, mm, mlat);
    break;

  case cfDDMMmmm:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%02d")_T(DEG)_T("%06.3f'"), EW[sign], dd, mlat);
    break;

  case cfDDdddd:
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%07.4f")_T(DEG), EW[sign], mlat);
    break;

  default:
    return false;
  }

  return true;
}

const TCHAR *
Units::GetUnitName(Units_t Unit)
{
  return UnitDescriptors[Unit].Name;
}

CoordinateFormats_t
Units::GetCoordinateFormat()
{
  return CoordinateFormat;
}

CoordinateFormats_t
Units::SetCoordinateFormat(CoordinateFormats_t NewFormat)
{
  CoordinateFormats_t last = CoordinateFormat;
  if (CoordinateFormat != NewFormat)
    CoordinateFormat = NewFormat;
  return last;
}

Units_t
Units::GetUserDistanceUnit()
{
  return DistanceUnit;
}

Units_t
Units::SetUserDistanceUnit(Units_t NewUnit)
{
  Units_t last = DistanceUnit;
  if (DistanceUnit != NewUnit)
    DistanceUnit = NewUnit;
  return last;
}

Units_t
Units::GetUserAltitudeUnit()
{
  return AltitudeUnit;
}

Units_t
Units::SetUserAltitudeUnit(Units_t NewUnit)
{
  Units_t last = AltitudeUnit;
  if (AltitudeUnit != NewUnit)
    AltitudeUnit = NewUnit;
  return last;
}

Units_t
Units::GetUserTemperatureUnit()
{
  return TemperatureUnit;
}

Units_t
Units::SetUserTemperatureUnit(Units_t NewUnit)
{
  Units_t last = TemperatureUnit;
  if (TemperatureUnit != NewUnit)
    TemperatureUnit = NewUnit;
  return last;
}

Units_t
Units::GetUserSpeedUnit()
{
  return SpeedUnit;
}

Units_t
Units::SetUserSpeedUnit(Units_t NewUnit)
{
  Units_t last = SpeedUnit;
  if (SpeedUnit != NewUnit)
    SpeedUnit = NewUnit;
  return last;
}

Units_t
Units::GetUserTaskSpeedUnit()
{
  return TaskSpeedUnit;
}

Units_t
Units::SetUserTaskSpeedUnit(Units_t NewUnit)
{
  Units_t last = TaskSpeedUnit;
  if (TaskSpeedUnit != NewUnit)
    TaskSpeedUnit = NewUnit;
  return last;
}

Units_t
Units::GetUserVerticalSpeedUnit()
{
  return VerticalSpeedUnit;
}

Units_t
Units::SetUserVerticalSpeedUnit(Units_t NewUnit)
{
  Units_t last = VerticalSpeedUnit;
  if (VerticalSpeedUnit != NewUnit)
    VerticalSpeedUnit = NewUnit;
  return last;
}

Units_t
Units::GetUserWindSpeedUnit()
{
  return WindSpeedUnit;
}

Units_t
Units::SetUserWindSpeedUnit(Units_t NewUnit)
{
  Units_t last = WindSpeedUnit;
  if (WindSpeedUnit != NewUnit)
    WindSpeedUnit = NewUnit;
  return last;
}

Units_t
Units::GetUserUnitByGroup(UnitGroup_t UnitGroup)
{
  switch (UnitGroup) {
  case ugNone:
    return unUndef;
  case ugDistance:
    return GetUserDistanceUnit();
  case ugAltitude:
    return GetUserAltitudeUnit();
  case ugTemperature:
    return GetUserTemperatureUnit();
  case ugHorizontalSpeed:
    return GetUserSpeedUnit();
  case ugVerticalSpeed:
    return GetUserVerticalSpeedUnit();
  case ugWindSpeed:
    return GetUserWindSpeedUnit();
  case ugTaskSpeed:
    return GetUserTaskSpeedUnit();
  default:
    return unUndef;
  }
}

const TCHAR *
Units::GetSpeedName()
{
  return GetUnitName(GetUserSpeedUnit());
}

const TCHAR *
Units::GetVerticalSpeedName()
{
  return GetUnitName(GetUserVerticalSpeedUnit());
}

const TCHAR *
Units::GetDistanceName()
{
  return GetUnitName(GetUserDistanceUnit());
}

const TCHAR *
Units::GetAltitudeName()
{
  return GetUnitName(GetUserAltitudeUnit());
}

const TCHAR *
Units::GetTemperatureName()
{
  return GetUnitName(GetUserTemperatureUnit());
}

const TCHAR *
Units::GetTaskSpeedName()
{
  return GetUnitName(GetUserTaskSpeedUnit());
}

bool
Units::FormatUserAltitude(fixed Altitude, TCHAR *Buffer, size_t size,
                          bool IncludeUnit)
{
  int prec;
  TCHAR sTmp[32];
  const UnitDescriptor_t *pU = &UnitDescriptors[AltitudeUnit];

  /// \todo rounding
  Altitude = Altitude * pU->ToUserFact; // + pU->ToUserOffset;

  // prec = 4-log10(Altitude);
  // prec = max(prec, 0);
  prec = 0;

  if (IncludeUnit)
    _stprintf(sTmp, _T("%.*f%s"), prec, (double)Altitude, pU->Name);
  else
    _stprintf(sTmp, _T("%.*f"), prec, (double)Altitude);

  if (_tcslen(sTmp) < size - 1) {
    _tcscpy(Buffer, sTmp);
    return true;
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size - 1] = '\0';
    return false;
  }
}

bool
Units::FormatAlternateUserAltitude(fixed Altitude, TCHAR *Buffer, size_t size,
                                   bool IncludeUnit)
{
  Units_t saveUnit = AltitudeUnit;
  bool res;

  if (saveUnit == unMeter)
    AltitudeUnit = unFeet;
  if (saveUnit == unFeet)
    AltitudeUnit = unMeter;

  res = FormatUserAltitude(Altitude, Buffer, size, IncludeUnit);

  AltitudeUnit = saveUnit;

  return res;
}

// JMW, what does this do?
// TB: It seems to be the same as FormatUserAltitude() but it includes the
//     sign (+/-) in the output (see _stprintf())
bool
Units::FormatUserArrival(fixed Altitude, TCHAR *Buffer, size_t size,
                         bool IncludeUnit)
{
  int prec;
  TCHAR sTmp[32];
  const UnitDescriptor_t *pU = &UnitDescriptors[AltitudeUnit];

  Altitude = Altitude * pU->ToUserFact; // + pU->ToUserOffset;

  // prec = 4-log10(Altitude);
  // prec = max(prec, 0);
  prec = 0;

  if (IncludeUnit)
    _stprintf(sTmp, _T("%+.*f%s"), prec, (double)Altitude, pU->Name);
  else
    _stprintf(sTmp, _T("%+.*f"), prec, (double)Altitude);

  if (_tcslen(sTmp) < size - 1) {
    _tcscpy(Buffer, sTmp);
    return true;
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size - 1] = '\0';
    return false;
  }
}

bool
Units::FormatUserDistance(fixed Distance, TCHAR *Buffer, size_t size,
                          bool IncludeUnit)
{
  int prec;
  fixed value;
  TCHAR sTmp[32];

  const UnitDescriptor_t *pU = &UnitDescriptors[DistanceUnit];

  value = Distance * pU->ToUserFact; // + pU->ToUserOffset;

  if (value >= fixed(100))
    prec = 0;
  else if (value > fixed_ten)
    prec = 1;
  else if (value > fixed_one)
    prec = 2;
  else if (!IncludeUnit)
    prec = 3;
  else {
    prec = 3;
    if (DistanceUnit == unKiloMeter) {
      prec = 0;
      pU = &UnitDescriptors[unMeter];
      value = Distance * pU->ToUserFact;
    }
    if (DistanceUnit == unNauticalMiles ||
        DistanceUnit == unStatuteMiles) {
      pU = &UnitDescriptors[unFeet];
      value = Distance * pU->ToUserFact;
      if (value < fixed(1000)) {
        prec = 0;
      } else {
        prec = 1;
        pU = &UnitDescriptors[DistanceUnit];
        value = Distance * pU->ToUserFact;
      }
    }
  }

  if (IncludeUnit)
    _stprintf(sTmp, _T("%.*f%s"), prec, (double)value, pU->Name);
  else
    _stprintf(sTmp, _T("%.*f"), prec, (double)value);

  if (_tcslen(sTmp) < size - 1) {
    _tcscpy(Buffer, sTmp);
    return true;
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size - 1] = '\0';
    return false;
  }
}

bool
Units::FormatUserMapScale(Units_t *Unit, fixed Distance, TCHAR *Buffer,
                          size_t size, bool IncludeUnit)
{
  int prec;
  fixed value;
  TCHAR sTmp[32];

  const UnitDescriptor_t *pU = &UnitDescriptors[DistanceUnit];

  if (Unit != NULL)
    *Unit = DistanceUnit;

  value = Distance * pU->ToUserFact; // + pU->ToUserOffset;

  if (value >= fixed(9.999))
    prec = 0;
  else if ((DistanceUnit == unKiloMeter && value >= fixed(0.999)) ||
           (DistanceUnit != unKiloMeter && value >= fixed(0.160)))
    prec = 1;
  else if (!IncludeUnit)
    prec = 2;
  else {
    prec = 2;
    if (DistanceUnit == unKiloMeter) {
      prec = 0;
      if (Unit != NULL)
        *Unit = unMeter;
      pU = &UnitDescriptors[unMeter];
      value = Distance * pU->ToUserFact;
    }
    if (DistanceUnit == unNauticalMiles ||
        DistanceUnit == unStatuteMiles) {
      prec = 0;
      if (Unit != NULL)
        *Unit = unFeet;
      pU = &UnitDescriptors[unFeet];
      value = Distance * pU->ToUserFact;
    }
  }

  if (IncludeUnit)
    _stprintf(sTmp, _T("%.*f%s"), prec, (double)value, pU->Name);
  else
    _stprintf(sTmp, _T("%.*f"), prec, (double)value);

  if (_tcslen(sTmp) < size - 1) {
    _tcscpy(Buffer, sTmp);
    return true;
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size - 1] = '\0';
    return false;
  }
}

bool
Units::FormatUserSpeed(fixed Speed, TCHAR *Buffer, size_t size,
                       bool IncludeUnit)
{
  int prec;
  TCHAR sTmp[32];
  const UnitDescriptor_t *pU = &UnitDescriptors[SpeedUnit];

  Speed = Speed * pU->ToUserFact;

  prec = 0;
  if (Speed < fixed(100))
    prec = 1;

  if (IncludeUnit)
    _stprintf(sTmp, _T("%.*f%s"), prec, (double)Speed, pU->Name);
  else
    _stprintf(sTmp, _T("%.*f"), prec, (double)Speed);

  if (_tcslen(sTmp) < size - 1) {
    _tcscpy(Buffer, sTmp);
    return true;
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size - 1] = '\0';
    return false;
  }
}

bool
Units::FormatUserVSpeed(fixed Speed, TCHAR *Buffer, size_t size,
                        bool IncludeUnit)
{
  TCHAR sTmp[32];
  const UnitDescriptor_t *pU = &UnitDescriptors[VerticalSpeedUnit];

  Speed = Speed * pU->ToUserFact;

  if (IncludeUnit)
    _stprintf(sTmp, _T("%+.1f%s"), (double)Speed, pU->Name);
  else
    _stprintf(sTmp, _T("%+.1f"), (double)Speed);

  if (_tcslen(sTmp) < size - 1) {
    _tcscpy(Buffer, sTmp);
    return true;
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size - 1] = '\0';
    return false;
  }
}

fixed
Units::ConvertUnits(fixed Value, Units_t From, Units_t To)
{
  return ToUserUnit(ToSysUnit(Value, From), To);
}

fixed
Units::ToUserUnit(fixed Value, Units_t Unit)
{
  const UnitDescriptor_t *pU = &UnitDescriptors[Unit];
  return Value * pU->ToUserFact + pU->ToUserOffset;
}

fixed
Units::ToSysUnit(fixed Value, Units_t Unit)
{
  const UnitDescriptor_t *pU = &UnitDescriptors[Unit];
  return (Value - pU->ToUserOffset) / pU->ToUserFact;
}

void
Units::TimeToText(TCHAR* text, int d)
{
  int hours, mins;
  bool negative = (d < 0);
  int dd = abs(d) % (3600 * 24);
  hours = (dd / 3600);
  mins = (dd / 60 - hours * 60);
  hours = hours % 24;
  if (negative)
    _stprintf(text, _T("-%02d:%02d"), hours, mins);
  else
    _stprintf(text, _T("%02d:%02d"), hours, mins);
}
