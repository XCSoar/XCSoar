#ifndef GEO_VECTOR_HPP
#define GEO_VECTOR_HPP

struct GEOPOINT;

bool operator != (const GEOPOINT&g1, const GEOPOINT &g2);

struct GeoVector {
  GeoVector(const double distance, const double bearing):
    Distance(distance),
    Bearing(bearing)
  {
  };
  GeoVector(const double distance):Distance(distance),Bearing(0.0)
  {
  }
  GeoVector(const GEOPOINT &source, const GEOPOINT &target,
            const bool is_average=true);
  double Distance;
  double Bearing;
};

bool operator != (const GeoVector&g1, const GeoVector &g2);

#endif
