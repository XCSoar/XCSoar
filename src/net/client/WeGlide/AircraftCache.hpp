// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <boost/json/fwd.hpp>

namespace WeGlide {

bool
LoadAircraftListCacheValue(boost::json::value &value);

void
StoreAircraftListCacheValue(const boost::json::value &value);

} // namespace WeGlide
