#ifndef DISTANCE_MEMENTO_HPP
#define DISTANCE_MEMENTO_HPP

#include "GeoPoint.hpp"
#include "GeoVector.hpp"

/**
 * Memento object to store results of previous distance calculations. 
 */
class DistanceMemento {
public:

  /**
   * Constructor, initialises all to zero. 
   */
  DistanceMemento():value(0.0) {};

  /**
   * Returns the distance from the origin to destination in meters, 
   * from previously saved value if input arguments are identical. 
   */
  double Distance(const GEOPOINT& _origin,
                  const GEOPOINT& _destination) const;
private:

  /**
   * Origin point of saved query 
   */
  mutable GEOPOINT origin;

  /**
   * Destination point of previous query 
   */
  mutable GEOPOINT destination;

  /**
   * Distance in meters saved from previous query 
   */
  mutable double value;
};


#endif
