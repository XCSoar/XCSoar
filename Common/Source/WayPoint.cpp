#include "WayPoint.hpp"

WAYPOINT *WayPointList = NULL;
WPCALC *WayPointCalc = NULL; // VENTA3 additional infos calculated, parallel to WPs
unsigned int NumberOfWayPoints = 0;
int WaypointsOutOfRange = 1; // include
