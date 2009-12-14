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
#ifndef AIRSPACE_WARNING_HPP
#define AIRSPACE_WARNING_HPP

#include "AbstractAirspace.hpp"

/**
 * Class to hold information about active airspace warnings
 */
class AirspaceWarning 
{
public:

  enum AirspaceWarningState {
    WARNING_CLEAR=0,
    WARNING_TASK,
    WARNING_FILTER,
    WARNING_GLIDE,
    WARNING_INSIDE
  };

/** 
 * Constructor.  All warnings uniquely refer to an airspace. 
 * 
 * @param the_airspace Airspace that this object will manage warnings for
 */
  AirspaceWarning(const AbstractAirspace& the_airspace):
    m_airspace(the_airspace),
    m_state(WARNING_CLEAR),
    m_state_last(WARNING_CLEAR) {};

/** 
 * Save warning state prior to performing update
 * 
 */
  void save_state();

/** 
 * Update warning state and solution vector
 * 
 * @param state New warning state
 * @param solution Intercept vector (to outside if currently inside, otherwise to inside)
 */
  void update_solution(AirspaceWarningState state,
                       AirspaceInterceptSolution& solution);

/** 
 * Determine whether accepting a warning of the supplied state
 * will upgraded or equal the severity of the warning.
 * 
 * @param state New warning state
 */
  bool state_upgraded(AirspaceWarningState state);

/** 
 * Access airspace managed by this object
 * 
 * @return Airspace 
 */
  const AbstractAirspace& get_airspace() const {
    return m_airspace;
  }

/** 
 * Update warnings, calculate whether this airspace still needs to
 * have warnings managed.
 * 
 * @return True if warning is still active
 */
  bool action_updates();

private:
  const AbstractAirspace& m_airspace;
  AirspaceWarningState m_state;
  AirspaceWarningState m_state_last;
  AirspaceInterceptSolution m_solution;
};

#endif
