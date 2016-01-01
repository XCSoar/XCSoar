/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Formatter/Units.hpp"
#include "Units/System.hpp"
#include "Units/Descriptor.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Math/Util.hpp"
#include "Util/StringFormat.hpp"

static void
FormatInteger(TCHAR *buffer,
              const double value, const Unit unit, bool include_unit,
              bool include_sign)
{
  const auto uvalue = Units::ToUserUnit(value, unit);
  const int ivalue = iround(uvalue);

  if (include_unit)
    StringFormatUnsafe(buffer, include_sign ? _T("%+d %s") : _T("%d %s"),
                       ivalue, Units::GetUnitName(unit));
  else
    StringFormatUnsafe(buffer, include_sign ? _T("%+d") : _T("%d"), ivalue);
}

void
FormatMass(TCHAR *buffer, double value, Unit unit,
           bool include_unit)
{
  FormatInteger(buffer, value, unit, include_unit, false);
}

void
FormatWingLoading(TCHAR *buffer, double value, Unit unit,
                  bool include_unit)
{
  const auto uvalue = Units::ToUserUnit(value, unit);
  int precision = uvalue > 20 ? 0 : 1;

    if (include_unit)
      _stprintf(buffer, _T("%.*f %s"), precision, (double)uvalue,
                Units::GetUnitName(unit));
    else
      _stprintf(buffer, _T("%.*f"), precision, (double)uvalue);
}

void
FormatAltitude(TCHAR *buffer, double value, Unit unit,
               bool include_unit)
{
  FormatInteger(buffer, value, unit, include_unit, false);
}

void
FormatRelativeAltitude(TCHAR *buffer, double value,
                       Unit unit, bool include_unit)
{
  FormatInteger(buffer, value, unit, include_unit, true);
}

void
FormatDistance(TCHAR *buffer, double value, Unit unit,
               bool include_unit, int precision)
{
  value = Units::ToUserUnit(value, unit);

  if (include_unit)
    StringFormatUnsafe(buffer, _T("%.*f %s"), precision, (double)value,
                       Units::GetUnitName(unit));
  else
    StringFormatUnsafe(buffer, _T("%.*f"), precision, (double)value);
}

gcc_const
static Unit
GetSmallerDistanceUnit(Unit unit)
{
  switch (unit) {
  case Unit::KILOMETER:
    return Unit::METER;

  case Unit::NAUTICAL_MILES:
  case Unit::STATUTE_MILES:
    return Unit::FEET;

  default:
    return unit;
  }
}

Unit
FormatSmallDistance(TCHAR *buffer, double value, Unit unit,
                    bool include_unit, int precision)
{
  unit = GetSmallerDistanceUnit(unit);
  value = Units::ToUserUnit(value, unit);

  if (include_unit)
    StringFormatUnsafe(buffer, _T("%.*f %s"), precision, (double)value,
                       Units::GetUnitName(unit));
  else
    StringFormatUnsafe(buffer, _T("%.*f"), precision, (double)value);

  return unit;
}

static Unit
GetBestDistanceUnit(double value, Unit unit, double threshold = 2500)
{
  Unit small_unit = GetSmallerDistanceUnit(unit);
  if (small_unit == unit)
    return unit;

  auto small_value = Units::ToUserUnit(value, small_unit);
  return small_value > threshold ? unit : small_unit;
}

static int
GetBestDistancePrecision(double value, Unit unit, double threshold = 100)
{
  value = Units::ToUserUnit(value, unit);
  if (value >= threshold)
    return 0;
  else if (value > threshold / 10)
    return 1;
  else
    return 2;
}

Unit
FormatDistanceSmart(TCHAR *buffer, double value, Unit unit,
                    bool include_unit, double small_unit_threshold,
                    double precision_threshold)
{
  unit = GetBestDistanceUnit(value, unit, small_unit_threshold);
  int precision = GetBestDistancePrecision(value, unit, precision_threshold);
  FormatDistance(buffer, value, unit, include_unit, precision);

  return unit;
}

void
FormatSpeed(TCHAR *buffer,
            double value, const Unit unit, bool include_unit, bool precision)
{
  value = Units::ToUserUnit(value, unit);

  const int prec = precision && value < 100;
  if (include_unit)
    StringFormatUnsafe(buffer, _T("%.*f %s"), prec, (double)value,
                       Units::GetUnitName(unit));
  else
    StringFormatUnsafe(buffer, _T("%.*f"), prec, (double)value);
}

const TCHAR*
GetVerticalSpeedFormat(Unit unit, bool include_unit, bool include_sign)
{
  static const TCHAR *const format[2][2][2]= {
    //      0 0 0       0 0 1            0 1 0          0 1 1
    { { _T("%.1f"), _T("%+.1f") }, { _T("%.1f %s"), _T("%+.1f %s") } },
    //      1 0 0       1 0 1            1 1 0          1 1 1
    { { _T("%.0f"), _T("%+.0f") }, { _T("%.0f %s"), _T("%+.0f %s") } }
  };

  return format[unit == Unit::FEET_PER_MINUTE]
               [include_unit == true]
               [include_sign == true];
}

double
GetVerticalSpeedStep(Unit unit)
{
  switch (unit) {
  case Unit::FEET_PER_MINUTE:
    return 10;
  case Unit::KNOTS:
    return 0.2;
  default:
    return 0.1;
  }
}

void
FormatVerticalSpeed(TCHAR *buffer, double value, Unit unit,
                    bool include_unit, bool include_sign)
{
  value = Units::ToUserUnit(value, unit);

  if (include_unit)
    StringFormatUnsafe(buffer,
                       GetVerticalSpeedFormat(unit, include_unit, include_sign),
                       (double)value, Units::GetUnitName(unit));
  else
    StringFormatUnsafe(buffer,
                       GetVerticalSpeedFormat(unit, include_unit, include_sign),
                       (double)value);
}

void
FormatTemperature(TCHAR *buffer, double value, Unit unit,
                  bool include_unit)
{
  value = Units::ToUserUnit(value, unit);

  if (include_unit)
    StringFormatUnsafe(buffer, _T("%.0f %s"),
                       (double)value, Units::GetUnitName(unit));
  else
    StringFormatUnsafe(buffer, _T("%.0f"), (double)value);
}

void
FormatPressure(TCHAR *buffer, AtmosphericPressure pressure,
               Unit unit, bool include_unit)
{
  auto _pressure = Units::ToUserUnit(pressure.GetHectoPascal(), unit);

  if (include_unit)
    StringFormatUnsafe(buffer, GetPressureFormat(unit, include_unit),
                       (double)_pressure,
                       Units::GetUnitName(unit));
  else
    StringFormatUnsafe(buffer, GetPressureFormat(unit, include_unit),
                       (double)_pressure);
}

const TCHAR*
GetPressureFormat(Unit unit, bool include_unit)
{
  if (include_unit)
    return unit == Unit::INCH_MERCURY ? _T("%.2f %s") : _T("%.f %s");
  else
    return unit == Unit::INCH_MERCURY ? _T("%.2f") : _T("%.f");
}

double
GetPressureStep(Unit unit)
{
  return unit == Unit::INCH_MERCURY ? 0.01 : 1.;
}
