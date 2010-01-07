#ifndef TEST_WAYPOINTS_HPP
#define TEST_WAYPOINTS_HPP

#include "Waypoint/Waypoints.hpp"

const Waypoint* lookup_waypoint(const Waypoints& waypoints, unsigned id);
bool setup_waypoints(Waypoints &waypoints, const unsigned n=150);

#endif
