// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "util/StaticString.hxx"

#include <span>
#include <vector>

class NLineReader;
class Path;

namespace MOSMIX {

/**
 * One station of the DWD MOSMIX forecast catalogue.
 *
 * The catalogue is a fixed-width text file published by the DWD; see
 * #ReadStationCatalogue() for where it comes from and how it is read.
 */
struct Station {
  /**
   * The station id as the catalogue spells it, which is also what the
   * forecast URL is built from.  Kept verbatim rather than parsed as
   * a number: most are five digits, but the catalogue also carries
   * entries like "P0641" and "Z936".
   */
  StaticString<8> id;

  GeoPoint location;

  constexpr bool IsDefined() const noexcept {
    return !id.empty();
  }
};

/**
 * Parse one line of the station catalogue.
 *
 * @return a station whose IsDefined() is false when the line is a
 * header, a separator or otherwise not a station
 */
[[gnu::pure]]
Station ParseStationLine(const char *line) noexcept;

/**
 * Read the DWD MOSMIX station catalogue, which lists about 5600
 * stations worldwide with their positions.  Published at
 * https://www.dwd.de/DE/leistungen/opendata/help/stationen/
 * mosmix_stationskatalog.cfg and about 300 kB, so it is downloaded
 * once and kept.
 *
 * Lines that do not parse are skipped: the file carries a two line
 * header, and a single bad row is no reason to lose the rest.
 */
std::vector<Station> ReadStationCatalogue(NLineReader &reader);

/**
 * Read the catalogue from a file.  Returns an empty vector if it
 * cannot be read.
 */
std::vector<Station> ReadStationCatalogue(Path path) noexcept;

/**
 * The station closest to @p location by great circle distance.
 *
 * @return nullptr if there are no stations
 */
[[gnu::pure]]
const Station *FindNearestStation(std::span<const Station> stations,
                                  const GeoPoint &location) noexcept;

} // namespace MOSMIX
