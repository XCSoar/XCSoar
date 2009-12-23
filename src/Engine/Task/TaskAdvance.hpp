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
 * Constructor.  Sets defaults to auto-mode
 */
  TaskAdvance();

/**
 * Advance mode
 */
  enum TaskAdvanceMode_t {
    ADVANCE_MANUAL =0,          /**< No automatic advance */
    ADVANCE_AUTO,               /**< Automatic, triggers as soon as condition satisfied */
    ADVANCE_ARM,                /**< Requires arming of trigger on each task point */
    ADVANCE_ARMSTART            /**< Requires arming of trigger before start, thereafter works as ADVANCE_AUTO */
  };

/** 
 * Set arming trigger
 * 
 * @param do_armed True to arm trigger, false to clear
 */
  void set_armed(const bool do_armed) 
    {
      armed = do_armed;
    }

/** 
 * Accessor for arm state
 * 
 * @return True if armed
 */
  bool is_armed() const 
    {
      return armed;
    }

/** 
 * Toggle arm state
 * 
 * @return Arm state after toggle
 */
  bool toggle_armed()
    {
      armed = !armed;
      return armed;
    }

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
  bool ready_to_advance(const TaskPoint &tp,
                        const AIRCRAFT_STATE &state,
                        const bool x_enter, 
                        const bool x_exit) const;

/** 
 * Set task advance mode
 * 
 * @param the_mode New task advance mode
 */
  void set_mode(TaskAdvanceMode_t the_mode) {
    mode = the_mode;
  }
 
private:
/** 
 * Determine whether mode allows auto-advance, without
 * knowledge about turnpoint or state characteristics
 * 
 * @return True if this mode allows auto-advance
 */
  bool mode_ready() const;

  bool armed;                   /**< arm state */
  TaskAdvanceMode_t mode;       /**< acive advance mode */
};


#endif
