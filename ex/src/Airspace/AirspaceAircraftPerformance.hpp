#ifndef AIRSPACE_AIRCRAFT_PERFORMANCE_HPP
#define AIRSPACE_AIRCRAFT_PERFORMANCE_HPP

#include "Math/fixed.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Util/AircraftStateFilter.hpp"

/**
 *  Class used for simplified/idealised performace
 *  of aircraft speed as a function of glide slope.
 *  \todo method to derive this data from glide polar
 *  \todo specialisation based on recent trajectory
 */
class AirspaceAircraftPerformance {
public:

/** 
 * Default constructor.
 * Note that search mechanism will fail if descent_rate and climb_rate are zero
 * without tolerance being positive.
 * 
 * @param tolerance Tolerance of vertical speeds (m/s)
 */
  AirspaceAircraftPerformance(const fixed tolerance=fixed_zero):m_tolerance_vertical(tolerance) {};

/** 
 * Set tolerance of vertical speeds
 * 
 * @param val New value of tolerance, positive (m/s)
 */
  void set_tolerance_vertical(const fixed val) {
    m_tolerance_vertical = val;
  }

/** 
 * Return nominal speed
 * 
 * @return Nominal cruise speed (m/s)
 */
  virtual fixed get_cruise_speed() const = 0;

/** 
 * Return nominal descent rate
 * 
 * @return Nominal descent speed (m/s, positive down)
 */
  virtual fixed get_cruise_descent() const = 0;

/** 
 * Return descent rate limit (above nominal descent rate) 
 * 
 * @return Max descent speed (m/s, positive down)
 */
  virtual fixed get_descent_rate() const = 0;

/** 
 * Return climb rate limit (above nominal descent rate) 
 * 
 * @return Max climb rate (m/s, positive up)
 */
  virtual fixed get_climb_rate() const = 0;

/** 
 * Return maximum speed achievable by this model
 * 
 * @return Speed (m/s)
 */
  virtual fixed get_max_speed() const = 0;

/** 
 * Find minimum intercept time to a point
 * 
 * @param distance Distance to point (m)
 * @param dh Height of observer from point (m)
 * 
 * @return Time to intercept (s) or -1 if failed
 */
  virtual fixed solution_general(const fixed& distance,
                                 const fixed& dh) const;

/** 
 * Find time to intercept a target with a height band, set distance
 * 
 * @param distance Lateral distance to travel (m)
 * @param altitude Altitude of observer (m)
 * @param base Height of base (m)
 * @param top  Height of top (m)
 * @param intercept_alt If intercept possible, this is the soonest height
 * 
 * @return Time of intercept (s)
 */
  fixed solution_vertical(const fixed& distance,
                          const fixed& altitude,
                          const fixed& base,
                          const fixed& top,
                          fixed& intercept_alt) const;

/** 
 * Find time to intercept a target with a distance band, set height 
 * 
 * @param distance_min Min distance to travel (m)
 * @param distance_max Max distance to travel (m)
 * @param altitude Altitude of observer (m)
 * @param h  Height of target (m)
 * @param intercept_distance If intercept possible, this is the distance to the soonest point
 * 
 * @return Time of intercept (s)
 */
  fixed solution_horizontal(const fixed& distance_min,
                            const fixed& distance_max,
                            const fixed& altitude,
                            const fixed& h,
                            fixed& intercept_distance) const;

protected:
  fixed m_tolerance_vertical; /**< Tolerance in vertical max speeds (m/s) */

private:
  virtual bool solution_exists(const fixed& distance_min,
                               const fixed& distance_max,
                               const fixed& h_min,
                               const fixed& h_max) const;
};


/** 
 * Simplified aircraft performance model used for testing of
 * airspace warning system with minimal dependencies.
 * 
 */
class AirspaceAircraftPerformanceSimple:
  public AirspaceAircraftPerformance 
{
public:
/** 
 * Constructor.  Initialises current to experimental values
 * 
 * \todo use something more sensible?
 */
  AirspaceAircraftPerformanceSimple():v_ld(30.0),s_ld(2.0),
                                      climb_rate(10.0),
                                      descent_rate(10.0)
    {};

  virtual fixed get_cruise_speed() const {
    return v_ld;
  }

  virtual fixed get_cruise_descent() const {
    return s_ld;
  }

  virtual fixed get_climb_rate() const {
    return climb_rate;
  }

  virtual fixed get_descent_rate() const {
    return s_ld;
  }

  virtual fixed get_max_speed() const {
    return v_ld;
  }

private:
  fixed v_ld;
  fixed s_ld;
  fixed climb_rate;
  fixed descent_rate;
};

/**
 * Specialisation of AirspaceAircraftPerformance
 * based on simplified theoretical MC cross-country speeds.
 * Assumes cruise at best LD (ignoring wind) for current MC setting,
 * climb rate at MC setting, with direct descent possible at sink rate
 * of cruise.
 */
class AirspaceAircraftPerformanceGlide: 
  public AirspaceAircraftPerformance
{
public:
/** 
 * Constructor.
 * 
 * @param polar Polar to take data from
 * 
 * @return Initialised object
 */
  AirspaceAircraftPerformanceGlide(const GlidePolar& polar):
    m_glide_polar(polar) {

  };

  virtual fixed get_cruise_speed() const {
    return m_glide_polar.get_VbestLD();
  }

  virtual fixed get_cruise_descent() const {
    return m_glide_polar.get_SbestLD();
  }

  virtual fixed get_climb_rate() const {
    return m_glide_polar.get_mc();
  }

  virtual fixed get_descent_rate() const {
    return m_glide_polar.get_Smax();
  }

  virtual fixed get_max_speed() const {
    return m_glide_polar.get_Vmax();
  }

private:
  const GlidePolar &m_glide_polar;
};


/**
 * Specialisation of AirspaceAircraftPerformance based on
 * low pass filtered aircraft state --- effectively producing
 * potential solutions at average speed in the averaged direction
 * at the averaged climb rate.
 */
class AirspaceAircraftPerformanceStateFilter: 
  public AirspaceAircraftPerformance
{
public:
/** 
 * Constructor.
 * 
 * @param filter Filter to retrieve state information from
 * 
 * @return Initialised object
 */
  AirspaceAircraftPerformanceStateFilter(const AircraftStateFilter& filter):
    AirspaceAircraftPerformance(0.01),
    m_state_filter(filter) {

  }

  fixed get_cruise_speed() const {
    return m_state_filter.get_speed();
  }

  fixed get_cruise_descent() const {
    return -m_state_filter.get_climb_rate();
  }

  fixed get_climb_rate() const {
    return fixed_zero;
  }

  fixed get_descent_rate() const {
    return fixed_zero;
  }

  fixed get_max_speed() const {
    return m_state_filter.get_speed();
  }

private:
  const AircraftStateFilter &m_state_filter;
};

#endif
