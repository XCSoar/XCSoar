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
#include "TaskBestMc.hpp"
#include <math.h>
#include "Util/Tolerances.hpp"
#include <algorithm>

TaskBestMc::TaskBestMc(const std::vector<OrderedTaskPoint*>& tps,
                       const unsigned activeTaskPoint,
                       const AIRCRAFT_STATE &_aircraft,
                       const GlidePolar &_gp,
                       const double _mc_min):
  ZeroFinder(_mc_min,10.0,TOLERANCE_BEST_MC),
  tm(tps,activeTaskPoint,_gp),
  aircraft(_aircraft) 
{
}

TaskBestMc::TaskBestMc(TaskPoint* tp,
                       const AIRCRAFT_STATE &_aircraft,
                       const GlidePolar &_gp):
  ZeroFinder(0.1,10.0,TOLERANCE_BEST_MC),
  tm(tp,_gp),
  aircraft(_aircraft) 
{
}

double TaskBestMc::f(const double mc) 
{
  tm.set_mc(max(0.001,mc));
  res = tm.glide_solution(aircraft);

  if (res.Vector.Distance>0) {
    return res.AltitudeDifference/res.Vector.Distance;
  } else {
    return res.AltitudeDifference;
  }
}

bool TaskBestMc::valid(const double mc) 
{
  double ff = f(mc);
  return (res.Solution== GlideResult::RESULT_OK) && (ff>=-tolerance*2.0);
}

double TaskBestMc::search(const double mc) 
{
  double a = find_zero(mc);
  if (!valid(a)) {
    return mc;
  } 
  return a;
}
