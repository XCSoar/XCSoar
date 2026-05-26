// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceAltitude.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Navigation/Aircraft.hpp"
#include "util/CharUtil.hxx"

#include <string_view>

using std::string_view_literals::operator""sv;

namespace {

// Match Units::unit_descriptors factors; keep Engine independent of Units.
static constexpr double FEET_TO_METERS = 1.0 / 3.2808399;
static constexpr double METERS_TO_FLIGHT_LEVEL = 0.032808399;
static constexpr double FLIGHT_LEVEL_TO_METERS = 1.0 / 0.032808399;

[[gnu::const]]
static double
AltitudeToMeters(const double value, const bool feet) noexcept
{
  return feet ? value * FEET_TO_METERS : value;
}

[[gnu::const]]
static double
MetersToFlightLevel(const double meters) noexcept
{
  return meters * METERS_TO_FLIGHT_LEVEL;
}

} // namespace

std::optional<AirspaceAltitude>
ParseAirspaceAltitude(StringParser<> &input,
                      const ParseAirspaceAltitudeOptions &options)
{
  bool feet_unit = true;
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
      feet_unit = true;
    } else if (input.SkipMatchIgnoreCase("MSL"sv) ||
               (options.accept_amsl &&
                input.SkipMatchIgnoreCase("AMSL"sv))) {
      type = MSL;
    } else if (input.front() == 'M' || input.front() == 'm') {
      feet_unit = false;
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
    altitude.altitude = value * FLIGHT_LEVEL_TO_METERS;
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

  value = AltitudeToMeters(value, feet_unit);
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
    altitude.flight_level = MetersToFlightLevel(value);
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
