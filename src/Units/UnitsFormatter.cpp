/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "Math/Angle.hpp"
#include "Atmosphere/Pressure.hpp"
#include "DateTime.hpp"
#include "Util/StringUtil.hpp"

#include <stdio.h>
#include <stdlib.h>

static void
FormatInteger(TCHAR *buffer, size_t size,
              const fixed value, const Unit unit, bool include_unit)
{
  const fixed uvalue = Units::ToUserUnit(value, unit);
  const int ivalue = iround(uvalue);

  if (include_unit)
    _sntprintf(buffer, size, _T("%d %s"), ivalue,
               Units::unit_descriptors[(unsigned)unit].name);
  else
    _sntprintf(buffer, size, _T("%d"), ivalue);
}

void
Units::FormatUserAltitude(fixed value, TCHAR *buffer, size_t size,
                          bool include_unit)
{
  FormatInteger(buffer, size, value, current.altitude_unit, include_unit);
}

gcc_const
static Unit
GetAlternateUnit(Unit unit)
{
  switch (unit) {
  case Unit::METER:
    return Unit::FEET;

  case Unit::FEET:
    return Unit::METER;

  default:
    return unit;
  }
}

void
Units::FormatAlternateUserAltitude(fixed value, TCHAR *buffer, size_t size,
                                   bool include_unit)
{
  FormatInteger(buffer, size, value, GetAlternateUnit(current.altitude_unit),
                include_unit);
}

// JMW, what does this do?
// TB: It seems to be the same as FormatUserAltitude() but it includes the
//     sign (+/-) in the output (see _stprintf())
void
Units::FormatUserArrival(fixed value, TCHAR *buffer, size_t size,
                         bool include_unit)
{
  const UnitDescriptor *descriptor =
      &unit_descriptors[(unsigned)current.altitude_unit];

  value = value * descriptor->factor_to_user; // + descriptor->ToUserOffset;

  if (include_unit)
    _sntprintf(buffer, size, _T("%+d %s"), iround(value), descriptor->name);
  else
    _sntprintf(buffer, size, _T("%+d"), iround(value));
}

void
Units::FormatUserDistance(fixed _value, TCHAR *buffer, size_t size,
                          bool include_unit)
{
  int prec;
  fixed value;

  const UnitDescriptor *descriptor =
      &unit_descriptors[(unsigned)current.distance_unit];

  value = _value * descriptor->factor_to_user; // + descriptor->ToUserOffset;

  if (value >= fixed(100))
    prec = 0;
  else if (value > fixed_ten)
    prec = 1;
  else if (!include_unit)
    prec = 2;
  else {
    prec = 2;
    if (current.distance_unit == Unit::KILOMETER) {
      prec = 0;
      descriptor = &unit_descriptors[(unsigned)Unit::METER];
      value = _value * descriptor->factor_to_user;
    }
    if (current.distance_unit == Unit::NAUTICAL_MILES ||
        current.distance_unit == Unit::STATUTE_MILES) {
      descriptor = &unit_descriptors[(unsigned)Unit::FEET];
      value = _value * descriptor->factor_to_user;
      if (value < fixed(1000)) {
        prec = 0;
      } else {
        prec = 1;
        descriptor = &unit_descriptors[(unsigned)current.distance_unit];
        value = _value * descriptor->factor_to_user;
      }
    }
  }

  if (include_unit)
    _sntprintf(buffer, size, _T("%.*f %s"), prec, (double)value, descriptor->name);
  else
    _sntprintf(buffer, size, _T("%.*f"), prec, (double)value);
}

void
Units::FormatUserMapScale(fixed _value, TCHAR *buffer,
                          size_t size, bool include_unit)
{
  int prec;
  fixed value;

  const UnitDescriptor *descriptor =
      &unit_descriptors[(unsigned)current.distance_unit];

  value = _value * descriptor->factor_to_user; // + descriptor->ToUserOffset;

  if (value >= fixed(9.999))
    prec = 0;
  else if ((current.distance_unit == Unit::KILOMETER && value >= fixed(0.999)) ||
           (current.distance_unit != Unit::KILOMETER && value >= fixed(0.160)))
    prec = 1;
  else if (!include_unit)
    prec = 2;
  else {
    prec = 2;
    if (current.distance_unit == Unit::KILOMETER) {
      prec = 0;
      descriptor = &unit_descriptors[(unsigned)Unit::METER];
      value = _value * descriptor->factor_to_user;
    }
    if (current.distance_unit == Unit::NAUTICAL_MILES ||
        current.distance_unit == Unit::STATUTE_MILES) {
      prec = 0;
      descriptor = &unit_descriptors[(unsigned)Unit::FEET];
      value = _value * descriptor->factor_to_user;
    }
  }

  if (include_unit)
    _sntprintf(buffer, size, _T("%.*f %s"), prec, (double)value, descriptor->name);
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
    _sntprintf(buffer, max_size, _T("%.*f %s"),
               prec, (double)value, unit.name);
  else
    _sntprintf(buffer, max_size, _T("%.*f"),
               prec, (double)value);
}

void
Units::FormatUserSpeed(fixed value, TCHAR *buffer, size_t size,
                       bool include_unit, bool precision)
{
  FormatSpeed(value, buffer, size,
              include_unit, precision,
              unit_descriptors[(unsigned)current.speed_unit]);
}

void
Units::FormatUserWindSpeed(fixed value, TCHAR *buffer, size_t size,
                           bool include_unit, bool precision)
{
  FormatSpeed(value, buffer, size,
              include_unit, precision,
              unit_descriptors[(unsigned)current.wind_speed_unit]);
}

void
Units::FormatUserTaskSpeed(fixed value, TCHAR *buffer, size_t max_size,
                           bool include_unit, bool precision)
{
  FormatSpeed(value, buffer, max_size,
              include_unit, precision,
              unit_descriptors[(unsigned)current.task_speed_unit]);
}

void
Units::FormatUserVSpeed(fixed value, TCHAR *buffer, size_t size,
                        bool include_unit)
{
  const UnitDescriptor *descriptor =
      &unit_descriptors[(unsigned)current.vertical_speed_unit];

  value = value * descriptor->factor_to_user;

  if (include_unit)
    _sntprintf(buffer, size, _T("%+.1f %s"), (double)value, descriptor->name);
  else
    _sntprintf(buffer, size, _T("%+.1f"), (double)value);
}

void
Units::FormatUserTemperature(fixed value, TCHAR *buffer, size_t size,
                             bool include_unit)
{
  value = ToUserTemperature(value);

  if (include_unit)
    _sntprintf(buffer, size, _T("%.0f %s"), (double)value,
               GetTemperatureName());
  else
    _sntprintf(buffer, size, _T("%.0f"), (double)value);
}

bool
Units::FormatUserPressure(AtmosphericPressure pressure, TCHAR *buffer,
                          size_t size, bool include_unit)
{
  TCHAR buffer2[16];
  const UnitDescriptor *descriptor =
      &unit_descriptors[(unsigned)current.pressure_unit];

  fixed _pressure = pressure.GetHectoPascal() * descriptor->factor_to_user;

  if (include_unit) {
    TCHAR format[8];
    _tcscpy(format, GetFormatUserPressure());
    _tcscat(format, _T(" %s") );
    _stprintf(buffer2, format, (double)_pressure, descriptor->name);
  } else
    _stprintf(buffer2, GetFormatUserPressure(),  (double)_pressure);

  if (_tcslen(buffer2) < size - 1) {
    _tcscpy(buffer, buffer2);
    return true;
  } else {
    CopyString(buffer, buffer2, size);
    return false;
  }
}

const TCHAR*
Units::GetFormatUserPressure()
{
  if (current.pressure_unit == Unit::INCH_MERCURY)
    return _T("%.2f");
  else
    return _T("%.f");
}

fixed
Units::PressureStep()
{
  if (current.pressure_unit == Unit::INCH_MERCURY)
    return fixed(0.01);
  else
    return fixed_one;
}

void
Units::TimeToTextHHMMSigned(TCHAR* buffer, int _time)
{
  bool negative = (_time < 0);
  const BrokenTime time = BrokenTime::FromSecondOfDayChecked(abs(_time));
  if (negative)
    _stprintf(buffer, _T("-%02u:%02u"), time.hour, time.minute);
  else
    _stprintf(buffer, _T("%02u:%02u"), time.hour, time.minute);
}

void
Units::TimeToTextSmart(TCHAR *buffer1, TCHAR *buffer2, int _time)
{
  if ((unsigned)abs(_time) >= 24u * 3600u) {
    _tcscpy(buffer1, _T(">24h"));
    buffer2[0] = '\0';
    return;
  }

  const BrokenTime time = BrokenTime::FromSecondOfDay(abs(_time));

  if (time.hour > 0) { // hh:mm, ss
    // Set Value
    _stprintf(buffer1, _T("%02u:%02u"), time.hour, time.minute);
    _stprintf(buffer2, _T("%02u"), time.second);

  } else { // mm:ss
    _stprintf(buffer1, _T("%02u:%02u"), time.minute, time.second);
      buffer2[0] = '\0';
  }
}
