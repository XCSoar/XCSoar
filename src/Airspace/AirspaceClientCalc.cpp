#include "Airspace/AirspaceClientCalc.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "TaskClient.hpp"

void 
AirspaceClientCalc::reset_warning(const AIRCRAFT_STATE& as)
{
  Poco::ScopedRWLock lock(mutex, true);
  airspace_warning.reset(as);
}

bool 
AirspaceClientCalc::update_warning(const AIRCRAFT_STATE& as)
{
  Poco::ScopedRWLock lock(mutex, true);
  TaskClient::lock();
  bool retval = airspace_warning.update(as);
  TaskClient::unlock();
  return retval;
}
