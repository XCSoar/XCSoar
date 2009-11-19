#ifndef TEST_WAYPOINTS_HPP
#define TEST_WAYPOINTS_HPP

#include "Waypoint/Waypoints.hpp"

const Waypoint* lookup_waypoint(const Waypoints& waypoints, unsigned id);
void setup_waypoints(Waypoints &waypoints);

#endif
