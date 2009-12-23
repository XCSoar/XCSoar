/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */
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
  m_alt(state.NavAltitude)
{
  reset(state);
}

void 
AircraftStateFilter::reset(const AIRCRAFT_STATE& state)
{
  m_state_last = state;

  m_x = fixed_zero;
  m_y = fixed_zero;
  m_alt = state.NavAltitude;

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
  m_alt = state.NavAltitude;

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

AIRCRAFT_STATE 
AircraftStateFilter::get_predicted_state(const fixed &in_time) const
{
  AIRCRAFT_STATE state_next = m_state_last;
  GeoVector vec(get_speed()*in_time, get_bearing());
  state_next.Location = vec.end_point(m_state_last.Location);
  state_next.NavAltitude = m_state_last.NavAltitude+get_climb_rate()*in_time;
  return state_next;
}
