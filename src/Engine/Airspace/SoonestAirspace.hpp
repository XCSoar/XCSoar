// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Ptr.hpp"
#include "Predicate/AirspacePredicate.hpp"
#include "time/FloatDuration.hxx"

class AbstractAirspace;
class Airspaces;
struct AircraftState;
class AirspaceAircraftPerformance;

[[gnu::pure]]
ConstAirspacePtr
FindSoonestAirspace(const Airspaces &airspaces,
                    const AircraftState &state,
                    const AirspaceAircraftPerformance &perf,
                    AirspacePredicate predicate,
                    const FloatDuration max_time = FloatDuration{1.0e6});
