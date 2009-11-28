#include "IntermediatePoint.hpp"

double
IntermediatePoint::get_elevation() const
{
  return m_elevation+m_task_behaviour.safety_height_terrain;
}
