// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/AltitudeReference.hpp"
#include "util/StringAPI.hxx"
#include "util/StringParser.hxx"

#include <optional>
#include <string>
#include <string_view>

class AtmosphericPressure;
struct AltitudeState;

/** Structure to hold airspace altitude boundary data */
struct AirspaceAltitude
{
  /**
   * Altitude AMSL (m) resolved from type.
   *
   * Only valid if reference==AltitudeReference::MSL.  If
   * reference==AltitudeReference::AGL, a fallback value will be
   * calculated derived from terrain height, if available.  If
   * reference==AltitudeReference::STD, a fallback value will be
   * calculated derived from QNH, if available.
   */
  double altitude;

  /**
   * Flight level (100ft) for FL-referenced boundary.
   *
   * Only valid if reference==AltitudeReference::STD.
   */
  double flight_level;

  /**
   * Height above terrain (m) for ground-referenced boundary.
   *
   * Only valid if reference==AltitudeReference::AGL.
   */
  double altitude_above_terrain;

  /** Type of airspace boundary */
  AltitudeReference reference;

  /**
   * Get Altitude AMSL (m) resolved from type.
   * For AGL types, this assumes the terrain height
   * is the terrain height at the aircraft.
   */
  [[gnu::pure]]
  double GetAltitude(const AltitudeState &state) const noexcept;

  /** Is this altitude reference at or above the aircraft state? */
  [[gnu::pure]]
  bool IsAbove(const AltitudeState &state, double margin = 0) const noexcept;

  /** Is this altitude reference at or below the aircraft state? */
  [[gnu::pure]]
  bool IsBelow(const AltitudeState &state, double margin = 0) const noexcept;

  /**
   * Test whether airspace boundary is the terrain
   *
   * @return True if this altitude limit is the terrain
   */
  constexpr bool IsTerrain() const noexcept {
    return reference == AltitudeReference::AGL &&
      altitude_above_terrain <= 0;
  }

  /**
   * Set height of terrain for AGL-referenced airspace;
   * this sets Altitude and must be called before AGL-referenced
   * airspace is considered initialised.
   *
   * @param alt Height of terrain at airspace center
   */
  void SetGroundLevel(double alt) noexcept;

  /**
   * Is it necessary to call SetGroundLevel() for this AirspaceAltitude?
   */
  constexpr bool NeedGroundLevel() const noexcept {
    return reference == AltitudeReference::AGL;
  }

  /**
   * Set atmospheric pressure (QNH) for flight-level based
   * airspace.  This sets Altitude and must be called before FL-referenced
   * airspace is considered initialised.
   *
   * @param press Atmospheric pressure model (to obtain QNH)
   */
  void SetFlightLevel(AtmosphericPressure press) noexcept;

  static constexpr bool SortHighest(const AirspaceAltitude &a,
                                    const AirspaceAltitude &b) noexcept {
    return a.altitude > b.altitude;
  }
};

struct ParseAirspaceAltitudeOptions {
  /** If true, unknown tokens make parsing fail instead of being skipped. */
  bool strict_unknown_tokens = false;

  /** If true, treat AMSL like MSL (used by NOTAM API strings). */
  bool accept_amsl = false;

  /**
   * Ceiling altitude [m] used for UNL / UNLIMITED.
   * OpenAir parsing historically uses 50000; NOTAM uses feet-converted value.
   */
  double unlimited_ceiling_m = 50000;
};

/**
 * Parse an altitude limit string into #AirspaceAltitude.
 *
 * OpenAir and NOTAM altitude tokens (FL, GND, SFC, MSL, AGL, FT, M, STD, UNL).
 *
 * @return Parsed altitude, or std::nullopt when @p options.strict_unknown_tokens
 * is true and an unknown token is encountered.
 */
[[nodiscard]] std::optional<AirspaceAltitude>
ParseAirspaceAltitude(StringParser<> &input,
                      const ParseAirspaceAltitudeOptions &options = {});

[[nodiscard]] inline std::optional<AirspaceAltitude>
ParseAirspaceAltitude(const std::string_view text,
                      const ParseAirspaceAltitudeOptions &options = {})
{
  if (text.empty())
    return std::nullopt;

  const std::string owned{text};
  StringParser<> input{owned.c_str()};
  return ParseAirspaceAltitude(input, options);
}
