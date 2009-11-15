#ifndef GEO_VECTOR_HPP
#define GEO_VECTOR_HPP

struct GEOPOINT;

bool operator != (const GEOPOINT&g1, const GEOPOINT &g2);

/**
 * A constant bearing vector in lat/lon coordinates.  
 * Should later be extended to handle
 * separately constant bearing and minimum-distance paths. 
 *
 */
struct GeoVector {
  /**
   * Constructor given supplied distance/bearing 
   */
  GeoVector(const double distance, const double bearing):
    Distance(distance),
    Bearing(bearing)
  {
  };

  /**
   * Dummy constructor given distance, 
   * used to allow GeoVector x=0 calls. 
   */
  GeoVector(const double distance):
    Distance(distance),
    Bearing(0.0)
  {
  }

  /**
   * Constructor given start and end location.  
   * Computes Distance/Bearing internally. 
   *
   * \todo
   * - handle is_average
   */
  GeoVector(const GEOPOINT &source, const GEOPOINT &target,
            const bool is_average=true);

  /**
   * Adds the distance component of a geovector 
   */
  GeoVector& operator+= (const GeoVector&g1) {
    Distance+= g1.Distance;
    return *this;
  };

  /**
   * Returns the end point of the geovector projected from the start point.  
   * Assumes constant bearing. 
   */
  GEOPOINT end_point(const GEOPOINT &source) const;

  /**
   * Returns the end point of the geovector projected from the start point.  
   * Assumes constand Bearing. 
   */
  GEOPOINT mid_point(const GEOPOINT &source) const;

  /**
   * Distance in meters 
   */
  double Distance;

  /**
   * Bearing in degrees (true north) 
   */
  double Bearing;
};

bool operator != (const GeoVector&g1, const GeoVector &g2);

#endif
