// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"
#include "Settings.hpp"

#include <boost/json/fwd.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class CurlGlobal;
class ProgressListener;
struct GeoPoint;

namespace Co { template<typename T> class Task; }

namespace NOTAMClient {

using KnownMap = std::unordered_map<std::string, std::string>;

/**
 * Response structure for NOTAM fetch operations.
 *
 * Contains the fetched NOTAMs,
 * a flag indicating whether it is an incremental (= delta) update,
 * any removed NOTAM IDs (for delta updates),
 * and the raw JSON.
 */
struct NOTAMResponse {
  std::vector<NOTAM> notams;
  bool is_delta = false;
  std::vector<std::string> removed_ids;
  std::string raw_json;
};

/**
 * Download NOTAMs, optionally using delta updates.
 *
 * @param curl HTTP client
 * @param settings NOTAM configuration
 * @param location Center point for the search
 * @param progress Progress reporting interface
 * @param known Map of NOTAM id -> lastUpdated to request delta updates,
 *              or nullptr to perform a full fetch.
 * @return Response containing NOTAMs, removed IDs, and delta status
 */
[[nodiscard]]
Co::Task<NOTAMResponse>
FetchNOTAMsResponse(
    CurlGlobal &curl,
    const NOTAMSettings &settings,
    const GeoPoint &location,
    ProgressListener &progress,
    const KnownMap *known);

/**
 * Parse GeoJSON NOTAM data from the API response
 * 
 * @param json_data Raw JSON response from the API
 * @return Vector of parsed NOTAM objects
 */
[[nodiscard]]
std::vector<NOTAM>
ParseNOTAMGeoJSON(const std::string &json_data);

/**
 * Parse GeoJSON NOTAM data from a JSON value
 *
 * @param json Parsed JSON response from the API
 * @return Vector of parsed NOTAM objects
 */
[[nodiscard]]
std::vector<NOTAM>
ParseNOTAMGeoJSON(const boost::json::value &json);

} // namespace NOTAMClient
