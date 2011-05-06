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
#include "DateTime.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

bool
Units::LongitudeToString(Angle Longitude, TCHAR *Buffer, gcc_unused size_t size)
{
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
Units::LatitudeToString(Angle Latitude, TCHAR *Buffer, gcc_unused size_t size)
{
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

bool
Units::FormatUserAltitude(fixed Altitude, TCHAR *Buffer, size_t size,
                          bool IncludeUnit)
{
  TCHAR sTmp[32];
  const UnitDescriptor_t *pU = &UnitDescriptors[Current.AltitudeUnit];

  /// \todo rounding
  Altitude = Altitude * pU->ToUserFact; // + pU->ToUserOffset;

  if (IncludeUnit)
    _stprintf(sTmp, _T("%d%s"), iround(Altitude), pU->Name);
  else
    _stprintf(sTmp, _T("%d"), iround(Altitude));

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
  Units_t saveUnit = Current.AltitudeUnit;
  bool res;

  if (saveUnit == unMeter)
    Current.AltitudeUnit = unFeet;
  if (saveUnit == unFeet)
    Current.AltitudeUnit = unMeter;

  res = FormatUserAltitude(Altitude, Buffer, size, IncludeUnit);

  Current.AltitudeUnit = saveUnit;

  return res;
}

// JMW, what does this do?
// TB: It seems to be the same as FormatUserAltitude() but it includes the
//     sign (+/-) in the output (see _stprintf())
bool
Units::FormatUserArrival(fixed Altitude, TCHAR *Buffer, size_t size,
                         bool IncludeUnit)
{
  TCHAR sTmp[32];
  const UnitDescriptor_t *pU = &UnitDescriptors[Current.AltitudeUnit];

  Altitude = Altitude * pU->ToUserFact; // + pU->ToUserOffset;

  if (IncludeUnit)
    _stprintf(sTmp, _T("%+d%s"), iround(Altitude), pU->Name);
  else
    _stprintf(sTmp, _T("%+d"), iround(Altitude));

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

  const UnitDescriptor_t *pU = &UnitDescriptors[Current.DistanceUnit];

  value = Distance * pU->ToUserFact; // + pU->ToUserOffset;

  if (value >= fixed(100))
    prec = 0;
  else if (value > fixed_ten)
    prec = 1;
  else if (!IncludeUnit)
    prec = 2;
  else {
    prec = 2;
    if (Current.DistanceUnit == unKiloMeter) {
      prec = 0;
      pU = &UnitDescriptors[unMeter];
      value = Distance * pU->ToUserFact;
    }
    if (Current.DistanceUnit == unNauticalMiles ||
        Current.DistanceUnit == unStatuteMiles) {
      pU = &UnitDescriptors[unFeet];
      value = Distance * pU->ToUserFact;
      if (value < fixed(1000)) {
        prec = 0;
      } else {
        prec = 1;
        pU = &UnitDescriptors[Current.DistanceUnit];
        value = Distance * pU->ToUserFact;
      }
    }
  }

  if (IncludeUnit)
    _stprintf(sTmp, _T("%.*f %s"), prec, (double)value, pU->Name);
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

  const UnitDescriptor_t *pU = &UnitDescriptors[Current.DistanceUnit];

  if (Unit != NULL)
    *Unit = Current.DistanceUnit;

  value = Distance * pU->ToUserFact; // + pU->ToUserOffset;

  if (value >= fixed(9.999))
    prec = 0;
  else if ((Current.DistanceUnit == unKiloMeter && value >= fixed(0.999)) ||
           (Current.DistanceUnit != unKiloMeter && value >= fixed(0.160)))
    prec = 1;
  else if (!IncludeUnit)
    prec = 2;
  else {
    prec = 2;
    if (Current.DistanceUnit == unKiloMeter) {
      prec = 0;
      if (Unit != NULL)
        *Unit = unMeter;
      pU = &UnitDescriptors[unMeter];
      value = Distance * pU->ToUserFact;
    }
    if (Current.DistanceUnit == unNauticalMiles ||
        Current.DistanceUnit == unStatuteMiles) {
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
  const UnitDescriptor_t *pU = &UnitDescriptors[Current.SpeedUnit];

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
  const UnitDescriptor_t *pU = &UnitDescriptors[Current.VerticalSpeedUnit];

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
