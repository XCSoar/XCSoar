#ifndef AIRSPACE_ALTITUDE_HPP
#define AIRSPACE_ALTITUDE_HPP

#include "Math/fixed.hpp"

class AtmosphericPressure;

typedef enum {abUndef, abMSL, abAGL, abFL} AirspaceAltBase_t;

struct AIRSPACE_ALT
{
  AIRSPACE_ALT():Altitude(fixed_zero),
                 FL(fixed_zero),
                 AGL(fixed_zero),
                 Base(abUndef) {};

  fixed Altitude;
  fixed FL;
  fixed AGL;
  AirspaceAltBase_t Base;

/** 
 * Set height of terrain for AGL-referenced airspace;
 * this sets Altitude and must be called before AGL-referenced
 * airspace is considered initialised.
 * 
 * @param alt Height of terrain at airspace center
 */
  void set_ground_level(const fixed alt);

/** 
 * Set atmospheric pressure (QNH) for flight-level based
 * airspace.  This sets Altitude and must be called before FL-referenced
 * airspace is considered initialised.
 * 
 * @param alt Height of terrain at airspace center
 */
  void set_flight_level(const AtmosphericPressure &press);
};

#endif
