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
#ifndef TASKADVANCE_HPP
#define TASKADVANCE_HPP

class TaskPoint;
struct AIRCRAFT_STATE;

/**
 * Class used to control advancement through an OrderedTask
 */
class TaskAdvance
{
public:

  /**
   * Enumeration of states the task advance mechanism can be in
   */
  enum TaskAdvanceState_t {
    MANUAL = 0,                 /**< Advance is manual (user must manually adjust) */
    AUTO,                       /**< Advance is auto (no user input required) */
    START_ARMED,                /**< Armed for start */
    START_DISARMED,             /**< Not yet armed for start (user must arm when ready) */
    TURN_ARMED,                 /**< Armed for a turn */
    TURN_DISARMED               /**< Not yet armed for turn (user must arm when ready) */
  };

/** 
 * Constructor.  Sets defaults to auto-mode
 */
  TaskAdvance();

/**
 * Resets as if never flown
 */
  virtual void reset();

/** 
 * Set arming trigger
 * 
 * @param do_armed True to arm trigger, false to clear
 */
  void set_armed(const bool do_armed) 
    {
      m_armed = do_armed;
      m_request_armed = false;
    }

/** 
 * Accessor for arm state
 * 
 * @return True if armed
 */
  bool is_armed() const 
    {
      return m_armed;
    }

/** 
 * Accessor for arm request state
 * 
 * @return True if arm requested
 */
  bool request_armed() const 
    {
      return m_request_armed;
    }

/** 
 * Toggle arm state
 * 
 * @return Arm state after toggle
 */
  bool toggle_armed()
    {
      m_armed = !m_armed;
      if (m_armed) {
        m_request_armed = false;
      }
      return m_armed;
    }

  /** 
   * Retrieve current advance state
   * 
   * @return Advance state
   */
  virtual TaskAdvanceState_t get_advance_state() const = 0;

/** 
 * Determine whether all conditions are satisfied for a turnpoint
 * to auto-advance based on condition of the turnpoint, transition
 * characteristics and advance mode.
 * 
 * @param tp The task point to check for satisfaction
 * @param state current aircraft state
 * @param x_enter whether this step transitioned enter to this tp
 * @param x_exit whether this step transitioned exit to this tp
 * 
 * @return true if this tp is ready to advance
 */
  virtual bool ready_to_advance(const TaskPoint &tp,
                                const AIRCRAFT_STATE &state,
                                const bool x_enter, 
                                const bool x_exit) = 0;

protected:

/** 
 * Determine whether, according to OZ entry, an AAT OZ is ready to advance
 * 
 * @param has_entered True if the aircraft has entered the OZ
 * @param close_to_target If true, will advance when aircraft is close to target
 * 
 * @return True if ready to advance
 */
  virtual bool aat_state_ready(const bool has_entered,
                               const bool close_to_target) const;

/** 
 * Determine whether state is satisfied for a turnpoint
 * 
 * @param tp The task point to check for satisfaction
 * @param state current aircraft state
 * @param x_enter whether this step transitioned enter to this tp
 * @param x_exit whether this step transitioned exit to this tp
 * 
 * @return true if this tp is ready to advance
 */
  bool state_ready(const TaskPoint &tp,
                   const AIRCRAFT_STATE &state,
                   const bool x_enter, 
                   const bool x_exit) const;

  bool m_armed;                   /**< arm state */
  bool m_request_armed;           /**< need to arm */
};


#endif
