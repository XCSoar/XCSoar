#ifndef AIRCRAFT_STATE_FILTER_HPP
#define AIRCRAFT_STATE_FILTER_HPP

#include "Filter.hpp"
#include "DiffFilter.hpp"
#include "Navigation/Aircraft.hpp"
#include "Math/fixed.hpp"

/**
 * Class for filtering aircraft state (location and altitude) 
 * in order to derive average speed, bearing and climb rate
 * 
 */
class AircraftStateFilter {
public:
  /** 
   * Constructor
   * 
   * @param state Start state
   * @param cutoff_wavelength -3db cutoff wavelength (s) of filters
   */
  AircraftStateFilter(const AIRCRAFT_STATE& state,
                      const double cutoff_wavelength=10.0);

/** 
 * Reset filters to initial state
 * 
 * @param state State to reset to
 */
  void reset(const AIRCRAFT_STATE& state);

/** 
 * Update the filters.  Expects time to have advanced;
 * if it has retreated, will reset the filter to the new state.
 * 
 * @param state New state
 */
  void update(const AIRCRAFT_STATE& state);

/** 
 * Re-design filter.  Used to adjust the time constant of
 * the low pass filter.  If this fails, the low pass filter will
 * pass all frequencies through.
 * 
 * @param cutoff_wavelength -3db filter wavelength (s)
 * 
 * @return True if design was successfull
 */
  bool design(const double cutoff_wavelength);

/** 
 * Return filtered speed
 * 
 * @return Speed (m/s)
 */
  fixed get_speed() const;

/** 
 * Return filtered track bearing
 * 
 * @return Track bearing (deg true north)
 */
  fixed get_bearing() const;

/** 
 * Return filtered climb rate
 * 
 * @return Climb rate (m/s)
 */
  fixed get_climb_rate() const;

private:
  DiffFilter m_df_x, m_df_y, m_df_alt;
  Filter m_lpf_x, m_lpf_y, m_lpf_alt;
  AIRCRAFT_STATE m_state_last;
  fixed m_x, m_y, m_alt;
  fixed m_vx, m_vy, m_vz;
};

#endif
