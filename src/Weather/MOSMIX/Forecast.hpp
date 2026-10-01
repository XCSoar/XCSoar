// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Atmosphere/Temperature.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>

class Reader;
struct BrokenDate;

namespace MOSMIX {

/**
 * The element of a MOSMIX forecast that carries the maximum
 * temperature.  DWD defines it as "Maximum temperature - within the
 * last 12 hours", so the value stamped 18:00 UTC is the maximum
 * forecast for the twelve hours before it -- the daylight half of the
 * day, which is the one a convection estimate is about.
 *
 * It is not the maximum of the hourly TTT series: MOS forecasts each
 * element in its own right, and TX regularly runs a degree above the
 * highest full-hour value because a maximum does not have to fall on
 * the hour.
 */
inline constexpr std::string_view MAXIMUM_TEMPERATURE_ELEMENT = "TX";

/** The hour of the timestep that carries the daytime maximum. */
inline constexpr unsigned MAXIMUM_TEMPERATURE_HOUR = 18;

/**
 * Read the forecast maximum temperature for the daytime window of
 * @p date out of a MOSMIX KML document.
 *
 * Reads only as far as it must: the timesteps stand at the head of
 * the document and #MAXIMUM_TEMPERATURE_ELEMENT early among the
 * forecasts, so of a typical 340 kB document about thirty are
 * touched.
 *
 * @return nothing if the document does not parse, carries no timestep
 * at #MAXIMUM_TEMPERATURE_HOUR on that date, or leaves the value
 * empty -- which the DWD does for most timesteps, since the element
 * is only stated twice a day
 */
std::optional<Temperature>
ReadForecastMaximum(Reader &reader, const BrokenDate &date);

/**
 * Read the maximum out of a .kmz held in memory.
 *
 * The archive carries exactly one member, so its local file header is
 * read for the offset of the deflate stream and the stream is
 * inflated from there.  ZipArchive is not used for this: it addresses
 * a member by name, and the name carries the run timestamp
 * (MOSMIX_L_2026092515_10738.kml), which the caller does not know --
 * and the file would have to be written out to open it at all.
 *
 * @return nothing when the bytes are not such an archive, or the
 * document inside does not carry the element for that date
 */
std::optional<Temperature>
ReadForecastMaximumFromKmz(std::span<const std::byte> kmz,
                           const BrokenDate &date) noexcept;

/**
 * Build the URL of a station's latest MOSMIX_L forecast.
 *
 * The DWD publishes a new run every six hours and keeps the newest
 * under a fixed name, so the URL does not change and a conditional
 * request can be answered with "not modified".
 */
/* not noexcept: fmt::format() allocates and can throw */
std::string MakeForecastURL(std::string_view station_id);

} // namespace MOSMIX
