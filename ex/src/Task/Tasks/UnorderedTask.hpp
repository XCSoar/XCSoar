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
#ifndef UNORDEREDTASK_H
#define UNORDEREDTASK_H

#include "AbstractTask.hpp"

/**
 *  Common class for all unordered task types
 */
class UnorderedTask: public AbstractTask
{
public:
  /** 
   * Base constructor.
   * 
   * @param te Task events callback class (shared among all tasks) 
   * @param tb Global task behaviour settings
   * @param ta Advance mechanism used for advancable tasks
   * @param gp Global glide polar used for navigation calculations
   * 
   */
  UnorderedTask(const TaskEvents &te, 
                const TaskBehaviour &tb,
                TaskAdvance &ta,
                GlidePolar &gp);

protected:

  bool check_task() const;

  double calc_mc_best(const AIRCRAFT_STATE &state_now);

  double calc_glide_required(const AIRCRAFT_STATE &state_now);

  void glide_solution_remaining(const AIRCRAFT_STATE &state_now, 
                                        GlideResult &total,
                                        GlideResult &leg);

  void glide_solution_travelled(const AIRCRAFT_STATE &state_now, 
                                        GlideResult &total,
                                        GlideResult &leg);

  void glide_solution_planned(const AIRCRAFT_STATE &state_now,
                                      GlideResult &total,
                                      GlideResult &leg,
                                      DistanceRemainingStat &total_remaining_effective,
                                      DistanceRemainingStat &leg_remaining_effective,
                                      const double total_t_elapsed,
                                      const double leg_t_elapsed);

  double scan_total_start_time(const AIRCRAFT_STATE &state_now);

  double scan_leg_start_time(const AIRCRAFT_STATE &state_now);

  fixed scan_distance_nominal();
  
  fixed scan_distance_planned();

  fixed scan_distance_remaining(const GEOPOINT &ref);

  fixed scan_distance_scored(const GEOPOINT &ref);

  fixed scan_distance_travelled(const GEOPOINT &ref);

  void scan_distance_minmax(const GEOPOINT &ref, 
                            bool full,
                            double *dmin, double *dmax);

  double calc_gradient(const AIRCRAFT_STATE &state_now);

}; 

#endif
