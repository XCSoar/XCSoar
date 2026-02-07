// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceFormatter.hpp"
#include "Engine/Airspace/AirspaceAltitude.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "Math/Util.hpp"
#include "util/StringFormat.hpp"

#include <string.h>

void
AirspaceFormatter::FormatAltitudeShort(char *buffer,
                                       const AirspaceAltitude &altitude,
                                       bool include_unit)
{
  switch (altitude.reference) {
  case AltitudeReference::AGL:
    if (altitude.altitude_above_terrain <= 0)
      strcpy(buffer, "GND");
    else
      if (include_unit)
        StringFormatUnsafe(buffer, "%d %s AGL",
                           iround(Units::ToUserAltitude(altitude.altitude_above_terrain)),
                           Units::GetAltitudeName());
      else
        StringFormatUnsafe(buffer, "%d AGL",
                           iround(Units::ToUserAltitude(altitude.altitude_above_terrain)));
    break;

  case AltitudeReference::STD:
    StringFormatUnsafe(buffer, "FL%d", iround(altitude.flight_level));
    break;

  case AltitudeReference::MSL:
    if (include_unit)
      StringFormatUnsafe(buffer, "%d %s",
                         iround(Units::ToUserAltitude(altitude.altitude)),
                         Units::GetAltitudeName());
    else
      StringFormatUnsafe(buffer, "%d",
                         iround(Units::ToUserAltitude(altitude.altitude)));
    break;
  }
}

void
AirspaceFormatter::FormatAltitude(char *buffer,
                                  const AirspaceAltitude &altitude)
{
  FormatAltitudeShort(buffer, altitude);

  if ((altitude.reference == AltitudeReference::MSL ||
       altitude.reference == AltitudeReference::AGL) &&
      Units::GetUserAltitudeUnit() == Unit::METER)
    /* additionally show airspace altitude in feet, because aviation
       charts usually print altitudes in feet */
    StringFormatUnsafe(buffer + strlen(buffer), " (%d %s)",
                       iround(Units::ToUserUnit(altitude.altitude, Unit::FEET)),
                       Units::GetUnitName(Unit::FEET));

  if (altitude.reference != AltitudeReference::MSL &&
      altitude.altitude > 0)
    StringFormatUnsafe(buffer + strlen(buffer), " %d %s",
                       iround(Units::ToUserAltitude(altitude.altitude)),
                       Units::GetAltitudeName());
}
