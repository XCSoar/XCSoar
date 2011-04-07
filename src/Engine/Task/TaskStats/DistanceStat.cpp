/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

DistanceStatComputer::DistanceStatComputer(DistanceStat &_data,
                                           const bool _is_positive)
  :data(_data),
  df(fixed_zero),
  v_lpf(fixed(400) / N_AV, false),
  is_positive(_is_positive)
{

}

void 
DistanceStatComputer::calc_incremental_speed(const fixed dt)
{  
  if ((dt+fixed_half>=fixed_one) && positive(data.distance)) {
    if (av_dist.update(data.distance)) {
      const fixed d_av = av_dist.average() / N_AV;
      av_dist.reset();

      fixed v_f = fixed_zero;
      for (unsigned i=0; i<(unsigned)(dt+fixed_half); ++i) {
        const fixed v = df.update(d_av);
        v_f = v_lpf.update(v);
      }
      data.speed_incremental = (is_positive? -v_f:v_f);
    }
  } else if (!positive(dt) || !positive(data.distance)) {    
    reset_incremental_speed();
  }
}

void
DistanceStatComputer::reset_incremental_speed()
{
  df.reset(fixed(data.distance),
           (is_positive ? -1 : 1) * fixed(data.speed));
  v_lpf.reset((is_positive ? -1 : 1) * fixed(data.speed));
  data.speed_incremental = fixed_zero; // data.speed;
  av_dist.reset();
}


void 
DistanceStat::calc_speed(fixed time)
{
  if (positive(time)) {
    speed = fixed(distance) / time;
  } else {
    speed = fixed_zero;
  }
}
