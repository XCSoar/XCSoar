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
#ifndef TASKSOLVETRAVELLED_HPP
#define TASKSOLVETRAVELLED_HPP

#include "TaskMacCreadyTravelled.hpp"
#include "Util/ZeroFinder.hpp"

/**
 *  Abstract class to solve for travelled time.
 */
class TaskSolveTravelled: 
  public ZeroFinder
{
public:
/** 
 * Constructor for ordered task points
 * 
 * @param tps Vector of ordered task points comprising the task
 * @param activeTaskPoint Current active task point in sequence
 * @param _aircraft Current aircraft state
 * @param gp Glide polar to copy for calculations
 * @param xmin Min value of search parameter
 * @param xmax Max value of search parameter
 */
  TaskSolveTravelled(const std::vector<OrderedTaskPoint*>& tps,
                     const unsigned activeTaskPoint,
                     const AIRCRAFT_STATE &_aircraft,
                     const GlidePolar &gp,
                     const fixed xmin,
                     const fixed xmax);
  virtual ~TaskSolveTravelled() {};

/**
 * Calls travelled calculator 
 *
 * @return Time error
 */
  fixed time_error();

/** 
 * Search for parameter value.
 * 
 * @param ce Default parameter value
 * 
 * @return Value producing same travelled time
 */
  virtual fixed search(const fixed ce);

protected:
  TaskMacCreadyTravelled tm; /**< Travelled calculator */
private:
  GlideResult res;
  const AIRCRAFT_STATE &aircraft;
  fixed inv_dt;
  fixed dt;
};

#endif
