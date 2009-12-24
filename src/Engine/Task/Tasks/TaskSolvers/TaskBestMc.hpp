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
#ifndef TASKBESTMC_HPP
#define TASKBESTMC_HPP

#include "TaskMacCreadyRemaining.hpp"
#include "Util/ZeroFinder.hpp"

/**
 *  Class to solve for MacCready value, being the highest MC value to produce a
 *  pure glide solution for the remainder of the task.
 *  
 * \todo
 * - f() fails if Mc too low for wind, need to account for failed solution
 *
 */
class TaskBestMc: 
  public ZeroFinder
{
public:
/** 
 * Constructor for ordered task points
 * 
 * @param tps Vector of ordered task points comprising the task
 * @param activeTaskPoint Current active task point in sequence
 * @param _aircraft Current aircraft state
 * @param _gp Glide polar to copy for calculations
 * @param _mc_min Minimum legal value of MacCready (m/s) in search
 */
  TaskBestMc(const std::vector<OrderedTaskPoint*>& tps,
             const unsigned activeTaskPoint,
             const AIRCRAFT_STATE &_aircraft,
             const GlidePolar &_gp,
             const fixed _mc_min=fixed_zero);
/** 
 * Constructor for single task points (non-ordered ones)
 * 
 * @param tp Task point comprising the task
 * @param _aircraft Current aircraft state
 * @param _gp Glide polar to copy for calculations
 */
  TaskBestMc(TaskPoint* tp,
             const AIRCRAFT_STATE &_aircraft,
             const GlidePolar &_gp);
  virtual ~TaskBestMc() {};

  virtual fixed f(const fixed mc);

/** 
 * Test validity of a solution given search parameter
 * 
 * @param mc Search parameter (MacCready setting (m/s))
 * 
 * @return True if solution is valid
 */
  virtual bool valid(const fixed mc);

/** 
 * Search for best MC.  If fails (MC=0 is below final glide), returns
 * default value.
 * 
 * @param mc Default MacCready value (m/s)
 * 
 * @return Best MC value found or default value if no solution
 */
  virtual fixed search(const fixed mc);
private:
  TaskMacCreadyRemaining tm;
  GlideResult res;
  const AIRCRAFT_STATE &aircraft;
};

#endif

