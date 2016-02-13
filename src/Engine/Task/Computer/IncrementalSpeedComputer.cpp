/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "IncrementalSpeedComputer.hpp"
#include "Task/Stats/DistanceStat.hpp"
#include "Math/Util.hpp"

IncrementalSpeedComputer::IncrementalSpeedComputer(const bool _is_positive)
  :df(0),
   v_lpf(400 / N_AV, false),
   is_positive(_is_positive) {}

void
IncrementalSpeedComputer::Compute(DistanceStat &data, const double time)
{
  if (!data.IsDefined() || time < 0 ||
      (last_time >= 0 && (time < last_time || time > last_time + 60))) {
    Reset(data);
    return;
  }

  if (last_time < 0) {
    last_time = time;
    return;
  }

  const auto dt = time - last_time;
  const unsigned seconds = uround(dt);
  if (seconds == 0)
    return;

  if (!av_dist.Update(data.distance))
    return;

  const auto d_av = av_dist.Average();
  av_dist.Reset();

  double v_f = 0;
  for (unsigned i = 0; i < seconds; ++i) {
    const auto v = df.Update(d_av);
    v_f = v_lpf.Update(v);
  }

  last_time += seconds;

  data.speed_incremental = (is_positive ? -v_f : v_f);
}

void
IncrementalSpeedComputer::Reset(DistanceStat &data)
{
  auto distance = data.IsDefined() ? data.GetDistance() : 0;
  auto speed = data.IsDefined() ? data.GetSpeed() : 0;

  df.Reset(distance, (is_positive ? -1 : 1) * speed);
  v_lpf.Reset((is_positive ? -1 : 1) * speed);
  data.speed_incremental = 0; // data.speed;
  av_dist.Reset();

  last_time = -1;
}
