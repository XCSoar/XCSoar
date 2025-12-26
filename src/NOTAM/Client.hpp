// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"
#include "Settings.hpp"

#include <string>
#include <vector>

class CurlGlobal;
class ProgressListener;
struct GeoPoint;

namespace Co { template<typename T> class Task; }

namespace NOTAMClient {

/**
 * Download NOTAMs from the enroute-data API
 * 
 * @param curl HTTP client
 * @param settings NOTAM configuration
 * @param location Center point for the search
 * @param progress Progress reporting interface
 * @return Vector of NOTAMs in the specified area
 */
Co::Task<std::vector<NOTAM>>
FetchNOTAMs(CurlGlobal &curl, const NOTAMSettings &settings, 
            const GeoPoint &location, ProgressListener &progress);

/**
 * Download raw GeoJSON from the enroute-data API
 * 
 * @param curl HTTP client
 * @param settings NOTAM configuration
 * @param location Center point for the search
 * @param progress Progress reporting interface
 * @return Raw GeoJSON response as string
 */
Co::Task<std::string>
FetchNOTAMsRaw(CurlGlobal &curl, const NOTAMSettings &settings, 
               const GeoPoint &location, ProgressListener &progress);

/**
 * Parse GeoJSON NOTAM data from the API response
 * 
 * @param json_data Raw JSON response from the API
 * @return Vector of parsed NOTAM objects
 */
std::vector<NOTAM>
ParseNOTAMGeoJSON(const std::string &json_data);

} // namespace NOTAMClient