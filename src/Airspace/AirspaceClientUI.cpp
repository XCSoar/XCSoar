#include "Airspace/AirspaceClientUI.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Airspace/AirspaceParser.hpp"
#include "Terrain/RasterTerrain.hpp"

void 
AirspaceClientUI::visit_warnings(AirspaceWarningVisitor& visitor) const
{
  Poco::ScopedRWLock lock(mutex);
  return airspace_warning.visit_warnings(visitor);
}

void 
AirspaceClientUI::visit_within_range(const GEOPOINT &loc, 
                                     const fixed range,
                                     AirspaceVisitor &visitor,
                                     const AirspacePredicate &predicate) const
{
  return airspaces.visit_within_range(loc, range, visitor, predicate);
}

void
AirspaceClientUI::clear()
{
  Poco::ScopedRWLock lock(mutex, true);
  airspace_warning.clear();
  airspaces.clear();
}

void
AirspaceClientUI::clear_warnings()
{
  Poco::ScopedRWLock lock(mutex, true);
  airspace_warning.acknowledge_all();
}

AirspaceWarning& 
AirspaceClientUI::get_warning(const AbstractAirspace& airspace)
{
  Poco::ScopedRWLock lock(mutex, true);
  return airspace_warning.get_warning(airspace);
}

const AirspaceWarning* 
AirspaceClientUI::get_warning(const unsigned index) const
{
  Poco::ScopedRWLock lock(mutex);
  return airspace_warning.get_warning(index);
}

size_t 
AirspaceClientUI::warning_size() const
{
  Poco::ScopedRWLock lock(mutex);
  return airspace_warning.size();
}

bool 
AirspaceClientUI::warning_empty() const
{
  Poco::ScopedRWLock lock(mutex);
  return airspace_warning.empty();
}

int 
AirspaceClientUI::get_warning_index(const AbstractAirspace& airspace) const
{
  Poco::ScopedRWLock lock(mutex);
  return airspace_warning.get_warning_index(airspace);
}

void 
AirspaceClientUI::acknowledge_day(const AbstractAirspace& airspace,
                                  const bool set)
{
  Poco::ScopedRWLock lock(mutex, true);
  airspace_warning.acknowledge_day(airspace, set);
}

void 
AirspaceClientUI::acknowledge_warning(const AbstractAirspace& airspace,
                                  const bool set)
{
  Poco::ScopedRWLock lock(mutex, true);
  airspace_warning.acknowledge_warning(airspace, set);
}

void 
AirspaceClientUI::acknowledge_inside(const AbstractAirspace& airspace,
                                  const bool set)
{
  Poco::ScopedRWLock lock(mutex, true);
  airspace_warning.acknowledge_inside(airspace, set);
}
