// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <boost/json/fwd.hpp>

#include <string>
#include <stdexcept>

namespace PureTrack {

struct Settings;
struct Sample;

struct InsertResponse {
  bool success = false;
  unsigned http_code = 0;
  unsigned trackers_received = 0;
  unsigned total_points_received = 0;
  unsigned total_points_inserted = 0;
};

std::string
BuildInsertRequestBody(const Settings &settings, const Sample &sample);

InsertResponse
ParseInsertResponse(unsigned status, const boost::json::value &body);

std::runtime_error
ResponseToException(unsigned status, const boost::json::value &body);

} // namespace PureTrack
