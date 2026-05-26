// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceAltitude.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Navigation/Aircraft.hpp"
#include "Units/System.hpp"
#include "util/CharUtil.hxx"

#include <string_view>

using std::string_view_literals::operator""sv;

std::optional<AirspaceAltitude>
ParseAirspaceAltitude(StringParser<> &input,
                      const ParseAirspaceAltitudeOptions &options)
{
  auto unit = Unit::FEET;
  enum { MSL, AGL, SFC, FL, STD, UNLIMITED } type = MSL;
  double value = 0;

  while (true) {
    input.Strip();

    if (input.IsEmpty())
      break;

    if (IsDigitASCII(input.front())) {
      if (auto x = input.ReadDouble())
        value = *x;
    } else if (input.SkipMatchIgnoreCase("GND"sv) ||
               input.SkipMatchIgnoreCase("AGL"sv)) {
      type = AGL;
    } else if (input.SkipMatchIgnoreCase("SFC"sv)) {
      type = SFC;
    } else if (input.SkipMatchIgnoreCase("FL"sv)) {
      type = FL;
    } else if (input.SkipMatchIgnoreCase("FT"sv)) {
      unit = Unit::FEET;
    } else if (input.SkipMatchIgnoreCase("MSL"sv) ||
               (options.accept_amsl &&
                input.SkipMatchIgnoreCase("AMSL"sv))) {
      type = MSL;
    } else if (input.front() == 'M' || input.front() == 'm') {
      unit = Unit::METER;
      input.Skip();
    } else if (input.SkipMatchIgnoreCase("STD"sv)) {
      type = STD;
    } else if (input.SkipMatchIgnoreCase("UNL"sv)) {
      type = UNLIMITED;
    } else if (options.strict_unknown_tokens) {
      return std::nullopt;
    } else {
      input.Skip();
    }
  }

  AirspaceAltitude altitude{};

  switch (type) {
  case FL:
    altitude.reference = AltitudeReference::STD;
    altitude.flight_level = value;
    altitude.altitude = Units::ToSysUnit(value, Unit::FLIGHT_LEVEL);
    return altitude;

  case UNLIMITED:
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = options.unlimited_ceiling_m;
    return altitude;

  case SFC:
    altitude.reference = AltitudeReference::AGL;
    altitude.altitude_above_terrain = -1;
    altitude.altitude = 0;
    return altitude;

  default:
    break;
  }

  value = Units::ToSysUnit(value, unit);
  switch (type) {
  case MSL:
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = value;
    return altitude;

  case AGL:
    altitude.reference = AltitudeReference::AGL;
    altitude.altitude_above_terrain = value;
    altitude.altitude = value;
    return altitude;

  case STD:
    altitude.reference = AltitudeReference::STD;
    altitude.flight_level = Units::ToUserUnit(value, Unit::FLIGHT_LEVEL);
    altitude.altitude = value;
    return altitude;

  default:
    altitude.reference = AltitudeReference::MSL;
    altitude.altitude = value;
    return altitude;
  }
}

void
AirspaceAltitude::SetFlightLevel(const AtmosphericPressure press) noexcept
{
  static constexpr double fl_feet_to_m(30.48);
  if (reference == AltitudeReference::STD)
    altitude = press.PressureAltitudeToQNHAltitude(flight_level * fl_feet_to_m);
}

void
AirspaceAltitude::SetGroundLevel(const double alt) noexcept
{
  if (reference == AltitudeReference::AGL)
    altitude = altitude_above_terrain + alt;
}

bool
AirspaceAltitude::IsAbove(const AltitudeState &state,
                          const double margin) const noexcept
{
  return GetAltitude(state) >= state.altitude - margin;
}

bool
AirspaceAltitude::IsBelow(const AltitudeState &state,
                          const double margin) const noexcept
{
  return GetAltitude(state) <= state.altitude + margin ||
    /* special case: GND is always "below" the aircraft, even if the
       aircraft's AGL altitude turns out to be negative due to terrain
       file inaccuracies */
    IsTerrain();
}

double
AirspaceAltitude::GetAltitude(const AltitudeState &state) const noexcept
{
  // TODO: check if state.altitude_agl is valid
  return reference == AltitudeReference::AGL
    ? altitude_above_terrain + (state.altitude - state.altitude_agl)
    : altitude;
}
