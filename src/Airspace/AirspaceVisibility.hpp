#ifndef AIRSPACE_VISIBILITY_HPP
#define AIRSPACE_VISIBILITY_HPP

#include "Airspace/AirspacePredicate.hpp"
#include "Math/fixed.hpp"

struct SETTINGS_COMPUTER;
struct ALTITUDE_STATE;

class AirspaceVisible: public AirspacePredicate
{
public:
  AirspaceVisible(const SETTINGS_COMPUTER& _settings,
                  const ALTITUDE_STATE& _state):
    m_settings(_settings),
    m_state(_state)
    {};

  virtual bool operator()( const AbstractAirspace& airspace ) const { 
    return parent_condition(airspace);
  }

  bool parent_condition(const AbstractAirspace& airspace) const {
    if (!type_visible(airspace)) {
      return false;
    }
    if (!altitude_visible(airspace)) {
      return false;
    }
    return true;
  }

/** 
 * Determine if airspace is visible based on observers' altitude
 * 
 * @param airspace Airspace to test
 * 
 * @return True if visible
 */
  bool altitude_visible(const AbstractAirspace& airspace) const;

/** 
 * Determine if airspace is visible based on type
 * 
 * @param airspace Airspace to test
 * 
 * @return True if visible
 */
  bool type_visible(const AbstractAirspace& airspace) const;

protected:
  const SETTINGS_COMPUTER &m_settings;
  const ALTITUDE_STATE& m_state;
};


#endif
