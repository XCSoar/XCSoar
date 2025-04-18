// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/Task.hxx"

#include <chrono>
#include <vector>

struct GeoPoint;
class CurlGlobal;

/**
 * Client for ThermalInfoMap (https://thermalmap.info/api-doc.php)
 */
namespace TIM {

struct Thermal;

Co::Task<std::vector<Thermal>>
GetThermals(CurlGlobal &curl, std::chrono::hours max_age,
            GeoPoint location, unsigned radius);

} // namespace TIM
