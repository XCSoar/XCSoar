// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/Wind/BucketWind.hpp"
#include "Computer/Wind/CirclingWind.hpp"
#include "FakeLogFile.hpp"
#include "NMEA/CirclingInfo.hpp"
#include "NMEA/MoreData.hpp"
#include "TestUtil.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

struct CircleConfig {
  double tas;
  SpeedVector wind;
  double period_s;
  double dt_s = 1;
  unsigned circles = 3;
  /** If < 360, stop after this many degrees of heading. */
  double heading_span_deg = 360;
};

struct Sample {
  TimeStamp time;
  Angle track;
  double gs;
};

static void
AppendCircle(std::vector<Sample> &out, const CircleConfig &cfg)
{
  const Angle wind_to = cfg.wind.bearing.Reciprocal();
  const double wind_east = cfg.wind.norm * wind_to.sin();
  const double wind_north = cfg.wind.norm * wind_to.cos();

  const double max_t =
    cfg.period_s * cfg.circles * (cfg.heading_span_deg / 360.);
  const unsigned n = unsigned(std::ceil(max_t / cfg.dt_s)) + 2;

  double t = 0;
  for (unsigned i = 0; i < n; ++i) {
    const Angle heading = Angle::Degrees(360 * t / cfg.period_s);
    const double air_east = cfg.tas * heading.sin();
    const double air_north = cfg.tas * heading.cos();
    const SpeedVector gs(air_east + wind_east, air_north + wind_north);

    out.push_back({
      TimeStamp{FloatDuration{t + 1}},
      gs.bearing,
      gs.norm,
    });
    t += cfg.dt_s;
    if (t > max_t)
      break;
  }
}

static MoreData
MakeFix(const Sample &s) noexcept
{
  MoreData info;
  info.Reset();
  info.clock = s.time;
  info.time = s.time;
  info.time_available.Update(info.clock);
  info.track = s.track;
  info.track_available.Update(info.clock);
  info.ground_speed = s.gs;
  info.ground_speed_available.Update(info.clock);
  return info;
}

static CirclingWind::Result
RunCircling(const std::vector<Sample> &samples, bool circling) noexcept
{
  CirclingWind circling_wind;
  circling_wind.Reset();

  CirclingInfo circling_info;
  circling_info.Clear();
  circling_info.circling = circling;

  CirclingWind::Result last(0);
  for (const auto &s : samples) {
    const auto result = circling_wind.NewSample(MakeFix(s), circling_info);
    if (result.IsValid())
      last = result;
  }
  return last;
}

static BucketWind::Result
RunBuckets(const std::vector<Sample> &samples) noexcept
{
  BucketWind buckets;
  buckets.Reset();
  for (const auto &s : samples)
    buckets.Update(s.time, s.track, s.gs);

  if (samples.empty())
    return {};
  return buckets.Fit(samples.back().time);
}

static bool
WindClose(SpeedVector got, SpeedVector want,
          double speed_tol, Angle bearing_tol) noexcept
{
  const auto bearing_error =
    std::fabs((got.bearing.AsBearing() - want.bearing).AsDelta().Degrees());

  if (std::fabs(got.norm - want.norm) > speed_tol ||
      bearing_error > bearing_tol.Degrees()) {
    diag("got %.2f m/s %.1f deg, want %.2f m/s %.1f deg "
         "(bearing error %.1f deg)",
         got.norm, got.bearing.AsBearing().Degrees(),
         want.norm, want.bearing.AsBearing().Degrees(),
         bearing_error);
    return false;
  }

  return true;
}

static void
PrintRow(const char *name, const CirclingWind::Result &cw,
         const BucketWind::Result &bw, const SpeedVector &truth) noexcept
{
  if (name != nullptr && std::strlen(name) > 0)
    std::printf("# %-18s  circling %s %5.1f m/s %4.0f°   "
                "bucket %s %5.1f m/s %4.0f°   truth %5.1f m/s %4.0f°\n",
                name,
                cw.IsValid() ? "ok" : "--",
                cw.IsValid() ? cw.wind.norm : 0,
                cw.IsValid() ? cw.wind.bearing.Degrees() : 0,
                bw.valid ? "ok" : "--",
                bw.valid ? bw.wind.norm : 0,
                bw.valid ? bw.wind.bearing.Degrees() : 0,
                truth.norm, truth.bearing.Degrees());
}

static void
ComparePGCircle() noexcept
{
  const SpeedVector wind(Angle::Degrees(90), 5);
  std::vector<Sample> samples;
  AppendCircle(samples, {
    .tas = 9,
    .wind = wind,
    .period_s = 25,
  });

  const auto cw = RunCircling(samples, true);
  const auto bw = RunBuckets(samples);
  PrintRow("PG full circle", cw, bw, wind);

  /* CirclingWind may reject a PG circle (issue #2905).  Buckets
     should still recover the wind from the same GPS traces. */
  ok1(bw.valid);
  ok1(WindClose(bw.wind, wind, 2, Angle::Degrees(40)));
}

static void
CompareGliderCircle() noexcept
{
  const SpeedVector wind(Angle::Degrees(270), 5);
  std::vector<Sample> samples;
  AppendCircle(samples, {
    .tas = 25,
    .wind = wind,
    .period_s = 20,
  });

  const auto cw = RunCircling(samples, true);
  const auto bw = RunBuckets(samples);
  PrintRow("glider circle", cw, bw, wind);

  ok1(cw.IsValid());
  ok1(bw.valid);
  ok1(WindClose(cw.wind, wind, 1.5, Angle::Degrees(30)));
  ok1(WindClose(bw.wind, wind, 1.5, Angle::Degrees(30)));
}

static void
ComparePartialArc() noexcept
{
  /* Not a full circle: CirclingWind should stay quiet; buckets can
     still fit if enough track angles are seen. */
  const SpeedVector wind(Angle::Degrees(90), 5);
  std::vector<Sample> samples;
  AppendCircle(samples, {
    .tas = 9,
    .wind = wind,
    .period_s = 25,
    .circles = 1,
    .heading_span_deg = 220,
  });

  const auto cw = RunCircling(samples, true);
  const auto bw = RunBuckets(samples);
  PrintRow("PG 220° arc", cw, bw, wind);

  ok1(!cw.IsValid());
  ok1(bw.valid);
  ok1(WindClose(bw.wind, wind, 2.5, Angle::Degrees(50)));
}

static void
CompareCruiseSweep() noexcept
{
  /* Slow heading change, no thermal: still fills buckets. */
  const SpeedVector wind(Angle::Degrees(0), 6);
  std::vector<Sample> samples;
  AppendCircle(samples, {
    .tas = 9,
    .wind = wind,
    .period_s = 180,
    .circles = 1,
  });

  const auto cw = RunCircling(samples, false);
  const auto bw = RunBuckets(samples);
  PrintRow("cruise 360° sweep", cw, bw, wind);

  ok1(!cw.IsValid());
  ok1(bw.valid);
  ok1(WindClose(bw.wind, wind, 2, Angle::Degrees(40)));
}

int
main()
{
  plan_tests(2 + 4 + 3 + 3);

  SetFakeLogFileQuiet(true);

  std::printf("# CirclingWind vs 72-bucket cosine fit (discussion #2918)\n");
  ComparePGCircle();
  CompareGliderCircle();
  ComparePartialArc();
  CompareCruiseSweep();

  return exit_status();
}
