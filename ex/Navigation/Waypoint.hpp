#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP


struct GEOPOINT {
  double Longitude;
  double Latitude;
};

struct FLAT_GEOPOINT {
  int Longitude;
  int Latitude;
};

struct WAYPOINT {
  GEOPOINT Location;
  double Altitude;
};


#endif
