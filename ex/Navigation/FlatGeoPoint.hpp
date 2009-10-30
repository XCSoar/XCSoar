#ifndef FLAT_GEOPOINT_HPP
#define FLAT_GEOPOINT_HPP

struct FLAT_GEOPOINT {
  FLAT_GEOPOINT():Longitude(0),Latitude(0) {};
  FLAT_GEOPOINT(const int x,
                const int y):
    Longitude(x),Latitude(y) {};
  int Longitude;
  int Latitude;

  unsigned distance_to(const FLAT_GEOPOINT &sp) const;
};

#endif
