#include "GeoVectorMemento.hpp"

GeoVector 
GeoVectorMemento::calc(const GEOPOINT& _origin,
                       const GEOPOINT& _destination) const
{
  if ((_origin != origin)||(_destination != destination)) {
    origin = _origin;
    destination = _destination;
    value = GeoVector(origin,destination);
  };
  return value;
}
