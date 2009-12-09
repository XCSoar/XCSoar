#ifndef AIRCRAFT_STATE_FILTER_HPP
#define AIRCRAFT_STATE_FILTER_HPP

#include "Filter.hpp"
#include "DiffFilter.hpp"
#include "Navigation/Aircraft.hpp"
#include "Math/fixed.hpp"

class AircraftStateFilter {
public:
  AircraftStateFilter(const AIRCRAFT_STATE& state,
                      const double cutoff_wavelength=10.0);

  void reset(const AIRCRAFT_STATE& state);
  void update(const AIRCRAFT_STATE& state);
  bool design(const double cutoff_wavelength);

  fixed get_speed() const;
  fixed get_bearing() const;
  fixed get_climb_rate() const;

private:
  DiffFilter m_df_x, m_df_y, m_df_alt;
  Filter m_lpf_x, m_lpf_y, m_lpf_alt;
  AIRCRAFT_STATE m_state_last;
  fixed m_x, m_y, m_alt;
  fixed m_vx, m_vy, m_vz;
};

#endif
