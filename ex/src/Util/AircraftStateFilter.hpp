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

/**
 * Calculate predicted state in future
 *
 * @param in_time Time step for extrapolation (s)
 * @return Predicted aircraft state in in_time seconds
 *
 */
  AIRCRAFT_STATE get_predicted_state(const fixed &in_time) const;

private:
  DiffFilter m_df_x, m_df_y, m_df_alt;
  Filter m_lpf_x, m_lpf_y, m_lpf_alt;
  AIRCRAFT_STATE m_state_last;
  fixed m_x, m_y, m_alt;
  fixed m_vx, m_vy, m_vz;
};

#endif
