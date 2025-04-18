// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IncrementalSpeedComputer.hpp"
#include "Task/Stats/DistanceStat.hpp"
#include "Math/Util.hpp"

IncrementalSpeedComputer::IncrementalSpeedComputer(const bool _is_positive)
  :df(0),
   v_lpf(400. / N_AV, false),
   is_positive(_is_positive) {}

void
IncrementalSpeedComputer::Compute(DistanceStat &data,
                                  const TimeStamp time) noexcept
{
  if (!data.IsDefined() || !time.IsDefined() ||
      (last_time.IsDefined() && (time < last_time || time > last_time + std::chrono::minutes{1}))) {
    Reset(data);
    return;
  }

  if (!last_time.IsDefined()) {
    last_time = time;
    return;
  }

  const auto dt = time - last_time;
  const unsigned seconds = uround(dt.count());
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

  last_time += std::chrono::seconds{seconds};

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

  last_time = TimeStamp::Undefined();
}
