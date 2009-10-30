#ifndef GEOVECTOR_MEMENTO_HPP
#define GEOVECTOR_MEMENTO_HPP

#include "GeoPoint.hpp"
#include "GeoVector.hpp"

class GeoVectorMemento {
public:
  GeoVectorMemento():value(0.0) {};

  GeoVector calc(const GEOPOINT& _origin,
                 const GEOPOINT& _destination) const;
private:
  mutable GEOPOINT origin;
  mutable GEOPOINT destination;
  mutable GeoVector value;
};

#endif
