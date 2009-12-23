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
#ifndef SCOREDTASKPOINT_HPP
#define SCOREDTASKPOINT_HPP

#include "SampledTaskPoint.hpp"

/**
 * Abstract specialisation of SampledTaskPoint to manage scoring
 * of progress along a task.  To do this, this class keeps track
 * of the aircraft state at entry and exit of the observation zone,
 * and provides methods to retrieve various reference locations used
 * in scoring calculations.
 *
 * \todo 
 * - implement reset() method
 * - better documentation of this class!
 */
class ScoredTaskPoint:
  public SampledTaskPoint
{
public:
/** 
 * Constructor.  Clears entry/exit states on instantiation.
 * 
 * @param tp Projection used for internal representations
 * @param wp Waypoint associated with the task point
 * @param tb Task Behaviour defining options (esp safety heights)
 * @param b_scored Whether distance within OZ is scored 
 * 
 * @return Partially initialised object
 */
  ScoredTaskPoint(const TaskProjection& tp,
                  const Waypoint & wp, 
                  const TaskBehaviour &tb,
                  const bool b_scored);

  virtual ~ScoredTaskPoint() {};

  /** 
   * Reset the task (as if never flown)
   * 
   */
  virtual void reset();

/** 
 * Test whether aircraft has entered the OZ
 * 
 * @return True if aircraft has entered the OZ
 */
  bool has_entered() const {
    return m_state_entered.Time>0;
  }

/** 
 * Test whether aircraft has exited the OZ
 * 
 * @return True if aircraft has exited the OZ
 */
  bool has_exited() const {
    return m_state_exited.Time>0;
  }

/** 
 * Get entry state of aircraft
 * 
 * @return State on entry
 */
  const AIRCRAFT_STATE& get_state_entered() const {
    return m_state_entered;
  }

/** 
 * Test whether aircraft has entered observation zone and
 * was previously outside; records this transition.
 * 
 * @param ref_now State current
 * @param ref_last State at last sample
 * 
 * @return True if observation zone is entered now
 */
  virtual bool transition_enter(const AIRCRAFT_STATE & ref_now, 
                                const AIRCRAFT_STATE & ref_last);

/** 
 * Test whether aircraft has exited observation zone and
 * was previously inside; records this transition.
 * 
 * @param ref_now State current
 * @param ref_last State at last sample
 * 
 * @return True if observation zone is exited now
 */
  virtual bool transition_exit(const AIRCRAFT_STATE & ref_now, 
                               const AIRCRAFT_STATE & ref_last);

/** 
 * Retrieve location to be used for the scored task.
 * 
 * @return Location 
 */
  const GEOPOINT& get_location_scored() const;

/** 
 * Retrieve location to be used for the task already travelled.
 * This is always the scored best location for prior-active task points.
 * 
 * @return Location 
 */
  const GEOPOINT& get_location_travelled() const;

/** 
 * Retrieve location to be used for remaining task.
 * This is either the reference location or target for post-active,
 * or scored best location for prior-active task points.
 * 
 * @return Location 
 */
  virtual const GEOPOINT& get_location_remaining() const;

private:
/** 
 * Set OZ entry state
 * 
 * @param state State at entry
 */
  void set_state_entered(const AIRCRAFT_STATE& state) {
    m_state_entered = state;
  }

/** 
 * Set OZ exit state
 * 
 * @param state State at exit
 */
  void set_state_exited(const AIRCRAFT_STATE& state) {
    m_state_exited = state;
  }

  virtual bool entry_precondition() const {
    return true;
  }
  virtual bool score_last_exit() const {
    return false;
  }
  virtual bool score_first_entry() const {
    return false;
  }

  AIRCRAFT_STATE m_state_entered;
  AIRCRAFT_STATE m_state_exited;
};

#endif
