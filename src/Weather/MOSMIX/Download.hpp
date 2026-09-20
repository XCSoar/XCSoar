// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Atmosphere/Temperature.hpp"
#include "co/InvokeTask.hxx"
#include "co/Task.hxx"

#include <chrono>
#include <optional>

class CurlGlobal;
class ProgressListener;
struct GeoPoint;
struct BrokenDate;

namespace MOSMIX {

/**
 * How long the station catalogue is kept before it is fetched again.
 *
 * It is about 300 kB, which is seventeen times a forecast, so it is
 * not something to re-read often.  Stations are added and retired
 * rarely enough that a month-old copy still points at one nearby.
 */
inline constexpr std::chrono::hours CATALOGUE_MAX_AGE{24 * 30};

/**
 * Fetch the forecast maximum temperature for the daytime window of
 * @p date at @p location.
 *
 * Downloads the station catalogue when it is missing or older than
 * #CATALOGUE_MAX_AGE, picks the station nearest @p location, and
 * fetches that station's forecast.
 *
 * Throws on a network error, so the caller can say what went wrong.
 *
 * @return nothing when the forecast parses but has no value for that
 * day -- a run may not reach it, and the element is stated only twice
 * a day
 */
Co::Task<std::optional<Temperature>>
CoFetchForecastMaximum(CurlGlobal &curl, const GeoPoint &location,
                       const BrokenDate &date, ProgressListener &progress);

} // namespace MOSMIX
