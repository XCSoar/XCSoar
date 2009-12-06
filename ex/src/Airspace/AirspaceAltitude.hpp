#ifndef AIRSPACE_ALTITUDE_HPP
#define AIRSPACE_ALTITUDE_HPP

#include "Math/fixed.hpp"

class AtmosphericPressure;

typedef enum {abUndef, abMSL, abAGL, abFL} AirspaceAltBase_t;

struct AIRSPACE_ALT
{
  fixed Altitude;
  fixed FL;
  fixed AGL;
  AirspaceAltBase_t Base;

  void set_ground_level(const fixed alt);
  void set_flight_level(const AtmosphericPressure &press);
};

#endif
