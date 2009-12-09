#include "AircraftStateFilter.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Math/Geometry.hpp"

AircraftStateFilter::AircraftStateFilter(const AIRCRAFT_STATE& state,
                                         const double cutoff_wavelength):
  m_df_x(0.0),
  m_df_y(0.0),
  m_df_alt(0.0),
  m_lpf_x(cutoff_wavelength),
  m_lpf_y(cutoff_wavelength),
  m_lpf_alt(cutoff_wavelength),
  m_x(fixed_zero),
  m_y(fixed_zero),
  m_alt(state.Altitude)
{
  reset(state);
}

void 
AircraftStateFilter::reset(const AIRCRAFT_STATE& state)
{
  m_state_last = state;

  m_x = fixed_zero;
  m_y = fixed_zero;
  m_alt = state.Altitude;

  m_vx = 0;
  m_vy = 0;
  m_vz = 0;

  m_lpf_x.reset(0.0);
  m_lpf_y.reset(0.0);
  m_lpf_alt.reset(0.0);
  m_df_x.reset(m_x,0.0);
  m_df_y.reset(m_y,0.0);
  m_df_alt.reset(m_alt, 0.0);
}

void 
AircraftStateFilter::update(const AIRCRAFT_STATE& state)
{
  // \todo
  // Should be able to use TrackBearing and Speed,
  // but for now, use low-level functions

  fixed dt = state.Time- m_state_last.Time;

  if (negative(dt)) {
    reset(state);
    return;
  }

  GeoVector vec(m_state_last.Location, state.Location);
  m_x+= sin(fixed_deg_to_rad*vec.Bearing)*vec.Distance;
  m_y+= cos(fixed_deg_to_rad*vec.Bearing)*vec.Distance;
  m_alt = state.Altitude;

  m_vx = m_lpf_x.update(m_df_x.update(m_x));
  m_vy = m_lpf_y.update(m_df_y.update(m_y));
  m_vz = m_lpf_alt.update(m_df_alt.update(m_alt));

  m_state_last = state;
}

fixed 
AircraftStateFilter::get_speed() const
{
  return sqrt(m_vx*m_vx+m_vy*m_vy);
}

fixed 
AircraftStateFilter::get_bearing() const
{
  return ::AngleLimit360(fixed_rad_to_deg*atan2(m_vx,m_vy));
}

fixed 
AircraftStateFilter::get_climb_rate() const
{
  return m_vz;
}


bool 
AircraftStateFilter::design(const double cutoff_wavelength)
{
  bool ok = true;
  ok &= m_lpf_x.design(cutoff_wavelength);
  ok &= m_lpf_y.design(cutoff_wavelength);
  ok &= m_lpf_alt.design(cutoff_wavelength);
  return ok;
}
