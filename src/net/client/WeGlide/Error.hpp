// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <boost/json/fwd.hpp>

#include <stdexcept>

namespace WeGlide {

[[gnu::pure]]
std::runtime_error
ResponseToException(unsigned status, const boost::json::value &body);

} // namespace WeGlide
