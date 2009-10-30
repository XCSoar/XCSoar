#ifndef DISTANCE_MEMENTO_HPP
#define DISTANCE_MEMENTO_HPP

#include "GeoPoint.hpp"
#include "GeoVector.hpp"

class DistanceMemento {
public:
  DistanceMemento():value(0.0) {};

  double Distance(const GEOPOINT& _origin,
                  const GEOPOINT& _destination) const;
private:
  mutable GEOPOINT origin;
  mutable GEOPOINT destination;
  mutable double value;
};


#endif
