/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

DistanceStatComputer::DistanceStatComputer(const bool _is_positive)
  :df(fixed(0)),
   v_lpf(fixed(400) / N_AV, false),
   is_positive(_is_positive) {}

void
DistanceStatComputer::CalcIncrementalSpeed(DistanceStat &data, const fixed dt)
{
  if ((dt + fixed(0.5) >= fixed(1)) && data.IsDefined()) {
    if (av_dist.Update(data.distance)) {
      const fixed d_av = av_dist.Average() / N_AV;
      av_dist.Reset();

      fixed v_f = fixed(0);
      for (unsigned i = 0; i < (unsigned)(dt + fixed(0.5)); ++i) {
        const fixed v = df.Update(d_av);
        v_f = v_lpf.Update(v);
      }
      data.speed_incremental = (is_positive ? -v_f : v_f);
    }
  } else if (!positive(dt) || !data.IsDefined()) {
    ResetIncrementalSpeed(data);
  }
}

void
DistanceStatComputer::ResetIncrementalSpeed(DistanceStat &data)
{
  fixed distance = data.IsDefined() ? data.GetDistance() : fixed(0);
  fixed speed = data.IsDefined() ? data.GetSpeed() : fixed(0);

  df.Reset(distance, (is_positive ? -1 : 1) * speed);
  v_lpf.Reset((is_positive ? -1 : 1) * speed);
  data.speed_incremental = fixed(0); // data.speed;
  av_dist.Reset();
}

void
DistanceStatComputer::CalcSpeed(DistanceStat &data, fixed time)
{
  if (positive(time) && data.IsDefined())
    data.speed = data.GetDistance() / time;
  else
    data.speed = fixed(0);
}
