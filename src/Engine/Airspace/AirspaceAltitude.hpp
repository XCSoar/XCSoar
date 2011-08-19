#ifndef AIRSPACE_ALTITUDE_HPP
#define AIRSPACE_ALTITUDE_HPP

#include "Math/fixed.hpp"
#include "Util/tstring.hpp"

class AtmosphericPressure;
struct AltitudeState;


/**
 *  Structure to hold airspace altitude boundary data
 */
struct AirspaceAltitude
{
  enum Type {
    UNDEFINED,
    MSL,
    AGL,
    FL
  };

  /** Altitude AMSL (m) resolved from type */
  fixed altitude;
  /** Flight level (100ft) for FL-referenced boundary */
  fixed flight_level;
  /** Height above terrain (m) for ground-referenced boundary */
  fixed altitude_above_terrain;
  /** Type of airspace boundary */
  Type type;

  /** 
   * Constructor.  Initialises to zero.
   * 
   * @return Initialised blank object
   */
  AirspaceAltitude():altitude(fixed_zero),
                 flight_level(fixed_zero),
                 altitude_above_terrain(fixed_zero),
                 type(UNDEFINED) {};

  /**
   * Get Altitude AMSL (m) resolved from type.  For AGL types, this assumes the terrain height
   * is the terrain height at the aircraft.
   */
  fixed GetAltitude(const AltitudeState& state) const;

  /**
   * Is this altitude reference at or above the aircraft state?
   */
  bool IsAbove(const AltitudeState& state, const fixed margin = fixed_zero) const;

  /**
   * Is this altitude reference at or below the aircraft state?
   */
  bool IsBelow(const AltitudeState& state, const fixed margin = fixed_zero) const;

/** 
 * Test whether airspace boundary is the terrain
 * 
 * @return True if this altitude limit is the terrain
 */
  bool IsTerrain() const {
    return (!positive(altitude_above_terrain) && (type==AGL));
  }

/** 
 * Set height of terrain for AGL-referenced airspace;
 * this sets Altitude and must be called before AGL-referenced
 * airspace is considered initialised.
 * 
 * @param alt Height of terrain at airspace center
 */
  void SetGroundLevel(const fixed alt);

/** 
 * Set atmospheric pressure (QNH) for flight-level based
 * airspace.  This sets Altitude and must be called before FL-referenced
 * airspace is considered initialised.
 * 
 * @param press Atmospheric pressure model (to obtain QNH)
 */
  void SetFlightLevel(const AtmosphericPressure &press);

/** 
 * Generate text form of airspace altitude boundary
 * 
 * @param concise Whether to produce short-form
 * 
 * @return String version of altitude reference
 */
  const tstring GetAsText(const bool concise = false) const;

/** 
 * Generate text form of airspace altitude boundary with user units
 * 
 * @param concise Whether to produce short-form
 * 
 * @return String version of altitude reference
 */
  const tstring GetAsTextUnits(const bool concise = false) const;

  static bool SortHighest(const AirspaceAltitude &a, const AirspaceAltitude &b) {
    return a.altitude > b.altitude;
  }
};

#endif
