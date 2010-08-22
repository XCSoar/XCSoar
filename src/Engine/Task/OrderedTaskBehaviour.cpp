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
#include "OrderedTaskBehaviour.hpp"
#include "TaskBehaviour.hpp"

OrderedTaskBehaviour::OrderedTaskBehaviour():
    task_scored(true),
    aat_min_time(3600*5.35),
    start_max_speed(60.0),
    start_max_height(0),
    start_max_height_ref(0),
    finish_min_height(0),
    fai_finish(false),
    min_points(2),
    max_points(20),
    homogeneous_tps(false),
    is_closed(false),
    start_requires_arm(false)
{

}

bool 
OrderedTaskBehaviour::check_start_speed(const AIRCRAFT_STATE &state,
                                        const TaskBehaviour& behaviour,
                                        const bool with_margin) const
{
  if (start_max_speed == fixed_zero)
    return true;

  const fixed margin = with_margin? behaviour.start_max_speed_margin:fixed_zero;

  return state.Speed <= start_max_speed+margin;
}


bool 
OrderedTaskBehaviour::check_start_height(const AIRCRAFT_STATE &state,
                                         const TaskBehaviour& behaviour,
                                         const bool with_margin) const
{
  if (start_max_height==0)
    return true;

  const unsigned margin = with_margin? behaviour.start_max_height_margin:0;

  if (start_max_height_ref>0) {
    return state.NavAltitude <= fixed(start_max_height + margin);
  } else {
    return state.AltitudeAGL <= fixed(start_max_height + margin);
  }
}


bool 
OrderedTaskBehaviour::check_finish_height(const AIRCRAFT_STATE &state) const
{
  if (finish_min_height==0)
    return true;

  return state.AltitudeAGL >= fixed(finish_min_height);
}
