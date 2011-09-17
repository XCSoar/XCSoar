#ifndef AIRSPACE_VISIBILITY_HPP
#define AIRSPACE_VISIBILITY_HPP

#include "Airspace/Predicate/AirspacePredicate.hpp"
#include "Math/fixed.hpp"

struct AirspaceComputerSettings;
struct AirspaceRendererSettings;
struct AltitudeState;

class AirspaceVisiblePredicate: public AirspacePredicate
{
protected:
  const AirspaceComputerSettings &computer_settings;
  const AirspaceRendererSettings &renderer_settings;
  const AltitudeState &state;

public:
  AirspaceVisiblePredicate(const AirspaceComputerSettings &_computer_settings,
                           const AirspaceRendererSettings &_renderer_settings,
                           const AltitudeState& _state)
    :computer_settings(_computer_settings),
     renderer_settings(_renderer_settings),
     state(_state) {}

  virtual bool condition(const AbstractAirspace &airspace) const {
    return IsTypeVisible(airspace) && IsAltitudeVisible(airspace);
  }

  /**
   * Determine if airspace is visible based on observers' altitude
   *
   * @param airspace Airspace to test
   *
   * @return True if visible
   */
  bool IsAltitudeVisible(const AbstractAirspace &airspace) const;

  /**
   * Determine if airspace is visible based on type
   *
   * @param airspace Airspace to test
   *
   * @return True if visible
   */
  bool IsTypeVisible(const AbstractAirspace &airspace) const;
};


#endif
