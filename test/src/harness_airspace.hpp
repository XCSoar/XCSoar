// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceWarningManager.hpp"

extern AirspaceWarningManager *airspace_warnings;

void setup_airspaces(Airspaces& airspaces, const GeoPoint &center, const unsigned n=150);

void scan_airspaces(const AircraftState state, 
                    const Airspaces& airspaces,
                    const AirspaceAircraftPerformance& perf,
                    bool do_report,
                    const GeoPoint &target);

bool test_airspace_extra(Airspaces &airspaces);


void print_warnings(const AirspaceWarningManager &airspace_warnings);
