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
#include "Navigation/Aircraft.hpp"
#include "TaskBehaviour.hpp"

void
TaskBehaviour::all_off()
{
  optimise_targets_range=false;
  optimise_targets_bearing=false;
  auto_mc= false;
  calc_cruise_efficiency=false;
  calc_glide_required=false;
  task_scored = false;
  enable_olc = false;
}

bool 
TaskBehaviour::check_start_speed(const AIRCRAFT_STATE &state) const
{
  if (positive(start_max_speed) && (state.Speed>start_max_speed)) {
    return false;
  } else {
    return true;
  }
}

TaskBehaviour::TaskBehaviour():
    optimise_targets_range(true),
    optimise_targets_bearing(true),
    auto_mc(false),
    calc_cruise_efficiency(true),
    calc_effective_mc(true),
    calc_glide_required(true),
    goto_nonlandable(true),
    task_scored(true),
    aat_min_time(3600*5.35),
    safety_height_terrain(150.0),
    safety_height_arrival(300.0),
    start_max_speed(60.0),
    risk_gamma(0.0),
    enable_olc(false)
{
}

