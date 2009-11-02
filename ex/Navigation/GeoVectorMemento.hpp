#ifndef GEOVECTOR_MEMENTO_HPP
#define GEOVECTOR_MEMENTO_HPP

#include "GeoPoint.hpp"
#include "GeoVector.hpp"

/**
 * Memento object to store results of previous GeoVector constructors. 
 */
class GeoVectorMemento 
{
public:

  /**
   * Constructor, initialises all to zero. 
   */
  GeoVectorMemento():
    value(0.0) {};

  /**
   * Returns a GeoVector object from the origin to destination, 
   * from previously saved value if input arguments are identical. 
   */
  GeoVector calc(const GEOPOINT& _origin,
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
   * GeoVector saved from previous query 
   */
  mutable GeoVector value;
};

#endif
