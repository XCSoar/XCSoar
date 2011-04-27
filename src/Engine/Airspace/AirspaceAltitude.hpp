#ifndef AIRSPACE_ALTITUDE_HPP
#define AIRSPACE_ALTITUDE_HPP

#include "Math/fixed.hpp"
#include "Util/tstring.hpp"

class AtmosphericPressure;
struct ALTITUDE_STATE;

enum AirspaceAltBase_t {
  abUndef,
  abMSL,
  abAGL,
  abFL
};

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
   * Get Altitude AMSL (m) resolved from type.  For AGL types, this assumes the terrain height
   * is the terrain height at the aircraft.
   */
  fixed get_altitude(const ALTITUDE_STATE& state) const;

  /**
   * Is this altitude reference at or above the aircraft state?
   */
  bool is_above  (const ALTITUDE_STATE& state, const fixed margin=fixed_zero) const;

  /**
   * Is this altitude reference at or below the aircraft state?
   */
  bool is_below  (const ALTITUDE_STATE& state, const fixed margin=fixed_zero) const;

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

/** 
 * Generate text form of airspace altitude boundary
 * 
 * @param concise Whether to produce short-form
 * 
 * @return String version of altitude reference
 */
  const tstring get_as_text(const bool concise=false) const;

/** 
 * Generate text form of airspace altitude boundary with user units
 * 
 * @param concise Whether to produce short-form
 * 
 * @return String version of altitude reference
 */
  const tstring get_as_text_units(const bool concise=false) const;

  static bool SortHighest(const AIRSPACE_ALT& a,
                          const AIRSPACE_ALT& b) {
    return a.Altitude > b.Altitude;
  }

};

#endif
