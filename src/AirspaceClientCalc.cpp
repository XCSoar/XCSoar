#include "AirspaceClientCalc.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceWarningManager.hpp"

void 
AirspaceClientCalc::reset_warning(const AIRCRAFT_STATE& as)
{
  ScopeLock lock(mutex);
  airspace_warning.reset(as);
}

bool 
AirspaceClientCalc::update_warning(const AIRCRAFT_STATE& as)
{
  ScopeLock lock(mutex);
  return airspace_warning.update(as);
}

void 
AirspaceClientCalc::set_flight_levels(const AtmosphericPressure &press)
{
  ScopeLock lock(mutex);
  airspaces.set_flight_levels(press);
}

