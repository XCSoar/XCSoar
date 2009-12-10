#ifndef AIRSPACE_ALTITUDE_HPP
#define AIRSPACE_ALTITUDE_HPP

#include "Math/fixed.hpp"

class AtmosphericPressure;

typedef enum {abUndef, abMSL, abAGL, abFL} AirspaceAltBase_t;

/**
 *  Structure to hold airspace altitude boundary data
 */
struct AIRSPACE_ALT
{
  /** 
   * Constructor.  Initialises to zero.
   * 
   * @return Initialised blank object
   */
  AIRSPACE_ALT():Altitude(fixed_zero),
                 FL(fixed_zero),
                 AGL(fixed_zero),
                 Base(abUndef) {};

  fixed Altitude; /**< Altitude AMSL (m) resolved from type */
  fixed FL; /**< Flight level (100ft) for FL-referenced boundary */
  fixed AGL; /**< Height above terrain (m) for ground-referenced boundary */
  AirspaceAltBase_t Base; /**< Type of airspace boundary */

/** 
 * Test whether airspace boundary is the terrain
 * 
 * @return True if this altitude limit is the terrain
 */
  bool is_terrain() const {
    return (!positive(AGL) && (Base==abAGL));
  }

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
 * @param press Atmospheric pressure model (to obtain QNH)
 */
  void set_flight_level(const AtmosphericPressure &press);
};

#endif
