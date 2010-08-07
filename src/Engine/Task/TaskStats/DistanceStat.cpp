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
#include "DistanceStat.hpp"
#include "TaskStats.hpp"

#define N_AV 3

DistanceStat::DistanceStat(const bool _is_positive):
  distance(0.0),
  speed(0.0),
  av_dist(N_AV),
  df(fixed_zero),
  v_lpf(fixed(600) / N_AV, false),
  is_positive(_is_positive)
{

}

void 
DistanceStat::calc_incremental_speed(const fixed dt)
{  
  if (positive(dt) && positive(distance)) {
    if (av_dist.update(distance)) {
      fixed d_av = av_dist.average();
      av_dist.reset();

      for (unsigned i=0; i<(unsigned)(dt); i++) {
        fixed v = df.update(d_av) / N_AV;
        double v_f = v_lpf.update(v);
        speed_incremental = (is_positive? -v_f:v_f);
      }
    }
  } else {    
    reset_incremental_speed();
  }
}

void
DistanceStat::reset_incremental_speed()
{
  df.reset(fixed(distance), (is_positive ? -1 : 1) * fixed(speed) * N_AV);
  v_lpf.reset((is_positive ? -1 : 1) * fixed(speed));
  speed_incremental = speed;
  av_dist.reset();
}


DistanceTravelledStat::DistanceTravelledStat():
  DistanceStat(false)
{
}

void 
DistanceRemainingStat::calc_speed(const ElementStat* es) 
{
  if (positive(es->TimeRemaining)) {
    speed = fixed(distance) / es->TimeRemaining;
  } else {
    speed = 0;
  }
}

void 
DistancePlannedStat::calc_speed(const ElementStat* es) 
{
  if (positive(es->TimePlanned)) {
    speed = fixed(distance) / es->TimePlanned;
  } else {
    speed = 0;
  }
}

void 
DistanceTravelledStat::calc_speed(const ElementStat* es) 
{
  if (positive(es->TimeElapsed)) {
    speed = fixed(distance) / es->TimeElapsed;
  } else {
    speed = 0;
  }
}
