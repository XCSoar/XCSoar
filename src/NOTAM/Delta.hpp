// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOTAM.hpp"
#include "Client.hpp"
#include "Geo/GeoPoint.hpp"

#include <boost/json/fwd.hpp>
#include <string>
#include <vector>

namespace NOTAMDelta {

[[nodiscard]] bool
IsApiResponseValid(const boost::json::value &value) noexcept;

[[nodiscard]] NOTAMClient::KnownMap
BuildKnownMap(const std::vector<struct NOTAM> &notams);

[[nodiscard]] bool
CanUseDelta(const NOTAMClient::KnownMap &known,
            const boost::json::value &cached_api,
            bool cached_api_valid,
            GeoPoint known_location,
            unsigned known_radius_km,
            GeoPoint location,
            unsigned radius_km) noexcept;

void
ApplyDeltaUpdates(std::vector<struct NOTAM> &current,
                  const std::vector<struct NOTAM> &updates,
                  const std::vector<std::string> &removed_ids);

[[nodiscard]] bool
ApplyDeltaToApi(boost::json::value &api_response,
                const boost::json::value &delta_response,
                const std::vector<std::string> &removed_ids);

} // namespace NOTAMDelta
