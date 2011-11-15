/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "Units/UnitsFormatter.hpp"
#include "Math/Angle.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "DateTime.hpp"
#include "Util/StringUtil.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

bool
Units::LongitudeToString(Angle Longitude, TCHAR *Buffer, gcc_unused size_t size)
{
  static gcc_constexpr_data TCHAR EW[] = _T("WEE");
  int dd, mm, ss;

  // Calculate Longitude sign
  int sign = Longitude.Sign()+1;
  double mlong(Longitude.AbsoluteDegrees());

  switch (coordinate_format) {
  case CF_DDMMSS:
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

  case CF_DDMMSS_SS:
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

  case CF_DDMM_MMM:
    // Calculate degrees
    dd = (int)mlong;
    // Calculate minutes
    mlong = (mlong - dd) * 60.0;
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%03d")_T(DEG)_T("%06.3f'"), EW[sign], dd, mlong);
    break;

  case CF_DD_DDDD:
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%08.4f")_T(DEG), EW[sign], mlong);
    break;

  default:
    return false;
  }

  return true;
}

bool
Units::LatitudeToString(Angle Latitude, TCHAR *Buffer, gcc_unused size_t size)
{
  static gcc_constexpr_data TCHAR EW[] = _T("SNN");
  int dd, mm, ss;

  // Calculate Latitude sign
  int sign = Latitude.Sign()+1;
  double mlat(Latitude.AbsoluteDegrees());

  switch (coordinate_format) {
  case CF_DDMMSS:
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

  case CF_DDMMSS_SS:
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

  case CF_DDMM_MMM:
    // Calculate degrees
    dd = (int)mlat;
    // Calculate minutes
    mlat = (mlat - dd) * 60.0;
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%02d")_T(DEG)_T("%06.3f'"), EW[sign], dd, mlat);
    break;

  case CF_DD_DDDD:
    // Save the string to the Buffer
    _stprintf(Buffer, _T("%c%07.4f")_T(DEG), EW[sign], mlat);
    break;

  default:
    return false;
  }

  return true;
}

TCHAR *
Units::FormatGeoPoint(const GeoPoint &location, TCHAR *buffer, size_t size)
{
  if (!LatitudeToString(location.latitude, buffer, size))
    return NULL;

  TCHAR *end = buffer + size, *p = buffer + _tcslen(buffer);
  if (p >= end)
    return NULL;

  *p++ = _T(' ');

  if (!LongitudeToString(location.longitude, p, end - p))
    return NULL;

  return buffer;
}

static void
FormatInteger(TCHAR *buffer, size_t size,
              const fixed value, const UnitDescriptor &unit, bool include_unit)
{
  const fixed uvalue = value * unit.factor_to_user + unit.offset_to_user;
  const int ivalue = iround(uvalue);

  if (include_unit)
    _sntprintf(buffer, size, _T("%d %s"), ivalue, unit.name);
  else
    _sntprintf(buffer, size, _T("%d"), ivalue);
}

void
Units::FormatUserAltitude(fixed Altitude, TCHAR *buffer, size_t size,
                          bool IncludeUnit)
{
  FormatInteger(buffer, size, Altitude,
                unit_descriptors[current.altitude_unit], IncludeUnit);
}

gcc_const
static Unit
GetAlternateUnit(Unit unit)
{
  switch (unit) {
  case unMeter:
    return unFeet;

  case unFeet:
    return unMeter;

  default:
    return unit;
  }
}

void
Units::FormatAlternateUserAltitude(fixed Altitude, TCHAR *buffer, size_t size,
                                   bool IncludeUnit)
{
  FormatInteger(buffer, size, Altitude,
                unit_descriptors[GetAlternateUnit(current.altitude_unit)],
                IncludeUnit);
}

// JMW, what does this do?
// TB: It seems to be the same as FormatUserAltitude() but it includes the
//     sign (+/-) in the output (see _stprintf())
void
Units::FormatUserArrival(fixed Altitude, TCHAR *buffer, size_t size,
                         bool IncludeUnit)
{
  const UnitDescriptor *pU = &unit_descriptors[current.altitude_unit];

  Altitude = Altitude * pU->factor_to_user; // + pU->ToUserOffset;

  if (IncludeUnit)
    _sntprintf(buffer, size, _T("%+d %s"), iround(Altitude), pU->name);
  else
    _sntprintf(buffer, size, _T("%+d"), iround(Altitude));
}

void
Units::FormatUserDistance(fixed Distance, TCHAR *buffer, size_t size,
                          bool IncludeUnit)
{
  int prec;
  fixed value;

  const UnitDescriptor *pU = &unit_descriptors[current.distance_unit];

  value = Distance * pU->factor_to_user; // + pU->ToUserOffset;

  if (value >= fixed(100))
    prec = 0;
  else if (value > fixed_ten)
    prec = 1;
  else if (!IncludeUnit)
    prec = 2;
  else {
    prec = 2;
    if (current.distance_unit == unKiloMeter) {
      prec = 0;
      pU = &unit_descriptors[unMeter];
      value = Distance * pU->factor_to_user;
    }
    if (current.distance_unit == unNauticalMiles ||
        current.distance_unit == unStatuteMiles) {
      pU = &unit_descriptors[unFeet];
      value = Distance * pU->factor_to_user;
      if (value < fixed(1000)) {
        prec = 0;
      } else {
        prec = 1;
        pU = &unit_descriptors[current.distance_unit];
        value = Distance * pU->factor_to_user;
      }
    }
  }

  if (IncludeUnit)
    _sntprintf(buffer, size, _T("%.*f %s"), prec, (double)value, pU->name);
  else
    _sntprintf(buffer, size, _T("%.*f"), prec, (double)value);
}

void
Units::FormatUserMapScale(fixed Distance, TCHAR *buffer,
                          size_t size, bool IncludeUnit)
{
  int prec;
  fixed value;

  const UnitDescriptor *pU = &unit_descriptors[current.distance_unit];

  value = Distance * pU->factor_to_user; // + pU->ToUserOffset;

  if (value >= fixed(9.999))
    prec = 0;
  else if ((current.distance_unit == unKiloMeter && value >= fixed(0.999)) ||
           (current.distance_unit != unKiloMeter && value >= fixed(0.160)))
    prec = 1;
  else if (!IncludeUnit)
    prec = 2;
  else {
    prec = 2;
    if (current.distance_unit == unKiloMeter) {
      prec = 0;
      pU = &unit_descriptors[unMeter];
      value = Distance * pU->factor_to_user;
    }
    if (current.distance_unit == unNauticalMiles ||
        current.distance_unit == unStatuteMiles) {
      prec = 0;
      pU = &unit_descriptors[unFeet];
      value = Distance * pU->factor_to_user;
    }
  }

  if (IncludeUnit)
    _sntprintf(buffer, size, _T("%.*f%s"), prec, (double)value, pU->name);
  else
    _sntprintf(buffer, size, _T("%.*f"), prec, (double)value);
}

static void
FormatSpeed(fixed value, TCHAR *buffer, size_t max_size,
            bool include_unit, bool precision,
            const UnitDescriptor &unit)
{
  value *= unit.factor_to_user;

  const int prec = precision && value < fixed(100);
  if (include_unit)
    _sntprintf(buffer, max_size, _T("%.*f%s"),
               prec, (double)value, unit.name);
  else
    _sntprintf(buffer, max_size, _T("%.*f"),
               prec, (double)value);
}

void
Units::FormatUserSpeed(fixed Speed, TCHAR *buffer, size_t size,
                       bool IncludeUnit, bool Precision)
{
  FormatSpeed(Speed, buffer, size,
              IncludeUnit, Precision,
              unit_descriptors[current.speed_unit]);
}

void
Units::FormatUserWindSpeed(fixed Speed, TCHAR *buffer, size_t size,
                           bool IncludeUnit, bool Precision)
{
  FormatSpeed(Speed, buffer, size,
              IncludeUnit, Precision,
              unit_descriptors[current.wind_speed_unit]);
}

void
Units::FormatUserVSpeed(fixed Speed, TCHAR *buffer, size_t size,
                        bool IncludeUnit)
{
  const UnitDescriptor *pU = &unit_descriptors[current.vertical_speed_unit];

  Speed = Speed * pU->factor_to_user;

  if (IncludeUnit)
    _sntprintf(buffer, size, _T("%+.1f%s"), (double)Speed, pU->name);
  else
    _sntprintf(buffer, size, _T("%+.1f"), (double)Speed);
}

void
Units::TimeToTextHHMMSigned(TCHAR* text, int d)
{
  bool negative = (d < 0);
  const BrokenTime t = BrokenTime::FromSecondOfDayChecked(abs(d));
  if (negative)
    _stprintf(text, _T("-%02u:%02u"), t.hour, t.minute);
  else
    _stprintf(text, _T("%02u:%02u"), t.hour, t.minute);
}

void
Units::TimeToTextSmart(TCHAR* HHMMSSSmart, TCHAR* SSSmart,int d)
{
  const BrokenTime t = BrokenTime::FromSecondOfDayChecked(abs(d));

  if (t.hour > 0) { // hh:mm, ss
    // Set Value
    _stprintf(HHMMSSSmart, _T("%02u:%02u"), t.hour, t.minute);
    _stprintf(SSSmart, _T("%02u"), t.second);

  } else { // mm:ss
    _stprintf(HHMMSSSmart, _T("%02u:%02u"), t.minute, t.second);
      SSSmart[0] = '\0';
  }
}
