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
#ifndef TASKEVENTS_HPP
#define TASKEVENTS_HPP

class TaskPoint;

/**
 * Class used to provide feedback based on events that can be triggered
 * by the task system.  Typically this would be specialised by the client
 * to hook up to end-user code. 
 */
class TaskEvents 
{
public:
  TaskEvents(): verbose(false) {};

/** 
 * Called when the aircraft enters a turnpoint observation zone
 * 
 * @param tp The turnpoint entered
 */
  virtual void transition_enter(const TaskPoint& tp) const;

/** 
 * Called when the aircraft exits a turnpoint observation zone
 * 
 * @param tp The turnpoint the aircraft has exited
 */
  virtual void transition_exit(const TaskPoint &tp) const;

/** 
 * Called when auto-advance has changed the active
 * task point in an ordered task
 * 
 * @param tp The turnpoint that is now active after auto-advance
 * @param i The task sequence number after auto-advance
 */
  virtual void active_advanced(const TaskPoint &tp, const int i) const;

/** 
 * Called when a taskpoint was altered internally.
 * This can happen when an AbortTask determines a better task
 * point is available, or (not yet impelemented) the glider
 * enters a different start point for multiple start points.
 * 
 * @param tp The new active taskpoint
 */
  virtual void active_changed(const TaskPoint &tp) const;

/** 
 * Called when aircraft speed is higher than set limit
 * according to task rules
 * 
 */
  virtual void warning_start_speed() const;
  
/** 
 * Called when a task is invalid due to improper construction
 * (e.g. no finish point etc) 
 * 
 * @param error Text of error message
 */
  virtual void construction_error(const char* error) const;

  bool verbose; /**< Option to enable basic output on events (for testing) */
};

#endif
