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
#include "TaskOptTarget.hpp"
#include <math.h>
#include "Util/Tolerances.hpp"

TaskOptTarget::TaskOptTarget(const std::vector<OrderedTaskPoint*>& tps,
                             const unsigned activeTaskPoint,
                             const AIRCRAFT_STATE &_aircraft,
                             const GlidePolar &_gp,
                             AATPoint &_tp_current,
                             StartPoint *_ts):
  ZeroFinder(fixed(0.02), fixed(0.98), fixed(TOLERANCE_OPT_TARGET)),
  tm(tps,activeTaskPoint,_gp),
  aircraft(_aircraft),
  tp_start(_ts),
  tp_current(_tp_current),
  iso(_tp_current)
{

}

fixed 
TaskOptTarget::f(const fixed p) 
{
  // set task targets
  set_target(p);

  res = tm.glide_solution(aircraft);

  return res.TimeElapsed;
}

bool 
TaskOptTarget::valid(const fixed tp) 
{
  f(tp);
  return (res.Solution== GlideResult::RESULT_OK);
}

fixed 
TaskOptTarget::search(const fixed tp) 
{
  if (tp_current.target_is_locked()) {
    // can't move, don't bother
    return -fixed_one;
  }
  if (iso.valid()) {
      tm.target_save();
    const fixed t = find_min(tp);
    if (!valid(t)) {
      // invalid, so restore old value
      tm.target_restore();
      return -fixed_one;
    } else {
      return t;
    }
  } else {
    return -fixed_one;
  }
}

void 
TaskOptTarget::set_target(const fixed p)
{
  const GEOPOINT loc = iso.parametric(min(xmax,max(xmin,p)));
  tp_current.set_target(loc);
  tp_start->scan_distance_remaining(aircraft.Location);
}
