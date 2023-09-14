// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FlightPhaseDetector.hpp"

#include <boost/json/fwd.hpp>

/**
 * Write JSON code for flight preformance statistics to the writer
 *
 * @param totals Flight statistics
 *
 * @see FlightPhaseDetector
 */
boost::json::object
WritePerformanceStats(const PhaseTotals &totals) noexcept;

/**
 * Write JSON code for list of flight phases to the writer
 *
 * @param phases List of flight phases
 *
 * @see FlightPhaseDetector
 */
boost::json::array
WritePhaseList(const PhaseList &phases) noexcept;
