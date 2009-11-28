#include "ObservationZoneClient.hpp"

void 
ObservationZoneClient::set_legs(const TaskPoint *previous,
                                const TaskPoint *current,
                                const TaskPoint *next)
{
  m_oz->set_legs(previous, current, next);
}
