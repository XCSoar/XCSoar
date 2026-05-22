// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "XCThermAPI.hpp"

#include <boost/json/value_to.hpp>

namespace boost::json {

XCThermAPI::ForecastSlot
tag_invoke(value_to_tag<XCThermAPI::ForecastSlot>, const value &jv);

} // namespace boost::json
