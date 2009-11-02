#include "DistanceMemento.hpp"

extern unsigned count_distbearing;

double 
DistanceMemento::Distance(const GEOPOINT& _origin,
                          const GEOPOINT& _destination) const
{
  if ((_origin != origin)||(_destination != destination)) {
    origin = _origin;
    destination = _destination;
    count_distbearing++;
    value = origin.distance(destination);
  };
  return value;
}
