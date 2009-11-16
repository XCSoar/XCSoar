#include "IntermediatePoint.hpp"

double
IntermediatePoint::getElevation() const
{
  return Elevation+task_behaviour.safety_height_terrain;
}
