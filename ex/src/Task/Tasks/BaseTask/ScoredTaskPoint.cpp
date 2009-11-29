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
#include "ScoredTaskPoint.hpp"

ScoredTaskPoint::ScoredTaskPoint(const TaskProjection& tp,
                                 const Waypoint & wp, 
                                 const TaskBehaviour &tb,
                                 const bool b_scored): 
  SampledTaskPoint(tp, wp, tb, b_scored)
{
  reset();
}

bool 
ScoredTaskPoint::transition_enter(const AIRCRAFT_STATE & ref_now, 
                                  const AIRCRAFT_STATE & ref_last)
{
  bool entered = ObservationZone::transition_enter(ref_now, ref_last);
  if (entered && entry_precondition()) {
    if (!score_first_entry() || !has_entered()) {
      m_state_entered = ref_now;
      return true;
    }
  }
  return entered;
}

bool 
ScoredTaskPoint::transition_exit(const AIRCRAFT_STATE & ref_now, 
                                  const AIRCRAFT_STATE & ref_last)
{
  bool exited = ObservationZone::transition_exit(ref_now, ref_last);
  if (exited) {
    if (score_last_exit()) {
      clear_sample_all_but_last(ref_last);
      m_state_entered = ref_last;
      m_state_exited = ref_now;
     } else {
      m_state_exited = ref_last;
    }
  }
  return exited;
}


const GEOPOINT &
ScoredTaskPoint::get_location_travelled() const
{
  return get_location_min();
}

const GEOPOINT &
ScoredTaskPoint::get_location_scored() const
{
  return get_location_min();
}

const GEOPOINT &
ScoredTaskPoint::get_location_remaining() const
{
  return get_location_min();
}

void 
ScoredTaskPoint::reset()
{
  SampledTaskPoint::reset();
  m_state_entered.Time = -1;
  m_state_exited.Time = -1;
}
