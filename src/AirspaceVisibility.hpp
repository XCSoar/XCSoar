#ifndef AIRSPACE_VISIBILITY_HPP
#define AIRSPACE_VISIBILITY_HPP

#include "Airspace/AirspacePredicate.hpp"
#include "Math/fixed.hpp"

class SETTINGS_COMPUTER;

class AirspaceVisible: public AirspacePredicate
{
public:
  AirspaceVisible(const SETTINGS_COMPUTER& _settings,
                  const fixed &_altitude):
    m_settings(_settings),
    m_altitude(_altitude)
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
 * @param alt Altitude of observer
 * @param settings Airspace altitude visibility settings
 * 
 * @return True if visible
 */
  bool altitude_visible(const AbstractAirspace& airspace) const;

/** 
 * Determine if airspace is visible based on type
 * 
 * @param settings Airspace visibility settings
 * 
 * @return True if visible
 */
  bool type_visible(const AbstractAirspace& airspace) const;

protected:
  const SETTINGS_COMPUTER &m_settings;
  const fixed m_altitude;
};


#endif
