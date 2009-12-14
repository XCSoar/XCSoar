#ifndef TEST_AIRSPACE_HPP
#define TEST_AIRSPACE_HPP

#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceVisitor.hpp"
#include "Airspace/AirspaceWarningManager.hpp"

void setup_airspaces(Airspaces& airspaces, const unsigned n=150);

void scan_airspaces(const AIRCRAFT_STATE state, 
                    const Airspaces& airspaces,
                    const AirspaceAircraftPerformance& perf,
                    bool do_report,
                    const GEOPOINT &target);

bool test_airspace_extra(Airspaces &airspaces);

#endif
