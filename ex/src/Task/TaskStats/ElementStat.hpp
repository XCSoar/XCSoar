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
#ifndef ELEMENT_STAT_HPP
#define ELEMENT_STAT_HPP

#include "GlideSolvers/GlideResult.hpp"
#ifdef DO_PRINT
#include <iostream>
#endif
#include "DistanceStat.hpp"
#include "TaskVario.hpp"

struct AIRCRAFT_STATE;

/**
 * Common task element statistics.  Used because we separately want to
 * track overall task statistics as well as that of the current leg.
 */
class ElementStat
{
public:
/** 
 * Constructor.  Initialises all to zero.
 * 
 */
  ElementStat();

  fixed TimeStarted; /**< Time (s) this element was started */
  fixed TimeElapsed; /**< Time (s) since element was started */
  fixed TimeRemaining; /**< Time (s) to element completion */
  fixed TimePlanned; /**< Time (s) of overall element */
  fixed gradient; /**< Gradient to element completion */

  DistanceRemainingStat remaining_effective; /**< Stats for effective remaining distance of element */
  DistanceRemainingStat remaining; /**< Stats for actual remaining distance of element */
  DistancePlannedStat planned; /**< Stats for overall element distance */
  DistanceTravelledStat travelled; /**< Stats for travelled distance in this element */

  GlideResult solution_planned; /**< Glide solution for planned element */
  GlideResult solution_travelled; /**< Glide solution for travelled element */
  GlideResult solution_remaining; /**< Glide solution for remaining element */
  GlideResult solution_mc0; /**< Glide solution for remaining element, MC=0 */

  TaskVario vario; /**< Rate of change of altitude difference (m/s) */ 

/** 
 * Calculate element times
 * 
 * @param ts Start time of this element (s)
 * @param state Aircraft state (to access time)
 */
  void set_times(const double ts, 
                 const AIRCRAFT_STATE& state);

/** 
 * Calculate element speeds.  Incremental speeds are
 * held at bulk speeds within first minute of elapsed time.
 *
 * @param dt Time step of sample (s)
 */
  void calc_speeds(const double dt);

/** 
 * Reset to uninitialised state, to supress calculation
 * of incremental speeds.
 */
  void reset();


/** 
 * Determine whether the task (or subtask) is able to be finished
 * (will fail if MC too low, wind too high etc)
 * 
 * @return True if can finish the task
 */
  bool achievable() const;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const ElementStat& es);
#endif

private:
  bool initialised;
};


#endif
