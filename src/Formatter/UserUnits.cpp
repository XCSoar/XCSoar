// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/UserUnits.hpp"
#include "Formatter/Units.hpp"
#include "Units/Units.hpp"
#include "Atmosphere/Pressure.hpp"

#include <stdio.h>

void
FormatUserWingLoading(double value, char *buffer, bool include_unit) noexcept
{
  FormatWingLoading(buffer, value, Units::GetUserWingLoadingUnit(),
                    include_unit);
}

void
FormatUserMass(double value, char *buffer, bool include_unit) noexcept
{
  FormatMass(buffer, value, Units::GetUserMassUnit(), include_unit);
}

void
FormatUserAltitude(double value, char *buffer, bool include_unit) noexcept
{
  FormatAltitude(buffer, value, Units::GetUserAltitudeUnit(), include_unit);
}

static constexpr Unit
GetAlternateAltitudeUnit(Unit unit) noexcept
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
FormatAlternateUserAltitude(double value, char *buffer, bool include_unit) noexcept
{
  FormatAltitude(buffer, value,
                 GetAlternateAltitudeUnit(Units::GetUserAltitudeUnit()),
                 include_unit);
}

void
FormatRelativeUserAltitude(double value, char *buffer, bool include_unit) noexcept
{
  FormatRelativeAltitude(buffer, value, Units::GetUserAltitudeUnit(),
                         include_unit);
}

void
FormatUserDistance(double value, char *buffer, bool include_unit, int precision) noexcept
{
  FormatDistance(buffer, value, Units::GetUserDistanceUnit(),
                 include_unit, precision);
}

Unit
FormatSmallUserDistance(char *buffer, double value, bool include_unit,
                        int precision) noexcept
{
  return FormatSmallDistance(buffer, value, Units::GetUserDistanceUnit(),
                             include_unit, precision);
}

Unit
FormatUserDistanceSmart(double value, char *buffer, bool include_unit,
                        double small_unit_threshold, double precision_threshold) noexcept
{
  return FormatDistanceSmart(buffer, value, Units::GetUserDistanceUnit(),
                             include_unit, small_unit_threshold,
                             precision_threshold);
}

Unit
FormatUserMapScale(double value, char *buffer, bool include_unit) noexcept
{
  return FormatDistanceSmart(buffer, value, Units::GetUserDistanceUnit(),
                             include_unit, 1000, 9.999);
}

void
FormatUserSpeed(double value, char *buffer, bool include_unit, bool precision) noexcept
{
  FormatSpeed(buffer, value, Units::GetUserSpeedUnit(), include_unit,
              precision);
}

void
FormatUserWindSpeed(double value, char *buffer, bool include_unit,
                    bool precision) noexcept
{
  FormatSpeed(buffer, value, Units::GetUserWindSpeedUnit(), include_unit,
              precision);
}

void
FormatUserTaskSpeed(double value, char *buffer, bool include_unit,
                    bool precision) noexcept
{
  FormatSpeed(buffer, value, Units::GetUserTaskSpeedUnit(), include_unit,
              precision);
}

const char *
GetUserVerticalSpeedFormat(bool include_unit, bool include_sign) noexcept
{
  return GetVerticalSpeedFormat(Units::GetUserVerticalSpeedUnit(), include_unit,
                                include_sign);
}

double
GetUserVerticalSpeedStep() noexcept
{
  return GetVerticalSpeedStep(Units::GetUserVerticalSpeedUnit());
}

void
FormatUserVerticalSpeed(double value, char *buffer, bool include_unit,
                        bool include_sign) noexcept
{
  FormatVerticalSpeed(buffer, value, Units::GetUserVerticalSpeedUnit(),
                      include_unit, include_sign);
}

void
FormatUserTemperature(double value, char *buffer, bool include_unit) noexcept
{
  FormatTemperature(buffer, value, Units::GetUserTemperatureUnit(),
                    include_unit);
}

void
FormatUserPressure(AtmosphericPressure pressure, char *buffer,
                   bool include_unit) noexcept
{
  FormatPressure(buffer, pressure, Units::GetUserPressureUnit(), include_unit);
}

const char *
GetUserPressureFormat(bool include_unit) noexcept
{
  return GetPressureFormat(Units::GetUserPressureUnit(), include_unit);
}

double
GetUserPressureStep() noexcept
{
  return GetPressureStep(Units::current.pressure_unit);
}
