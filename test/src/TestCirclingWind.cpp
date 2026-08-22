// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/Wind/CirclingWind.hpp"
#include "FakeLogFile.hpp"
#include "NMEA/CirclingInfo.hpp"
#include "NMEA/MoreData.hpp"
#include "TestUtil.hpp"

#include <cmath>

/**
 * Feed CirclingWind a constant-rate heading circle in a uniform wind.
 *
 * Ground track and ground speed are the GPS observables (air vector
 * plus wind).  No gyroscope.  Optional true airspeed, as a typical
 * vario would provide; paragliders usually have none.
 */
struct CircleConfig {
  double tas;
  SpeedVector wind;
  double period_s;
  double dt_s = 1;
  /** Alternate samples by ± this many seconds (replay / GPS jitter). */
  double dt_jitter_s = 0;
  unsigned circles = 3;
  bool report_tas = false;
  /** Polar min-sink TAS [m/s]; 0 = do not weight quality. */
  double vmin = 0;
};

static CirclingWind::Result
FeedCircle(const CircleConfig &cfg) noexcept
{
  CirclingWind circling_wind;
  circling_wind.Reset();

  CirclingInfo circling;
  circling.Clear();
  circling.circling = true;

  CirclingWind::Result last(0);

  const Angle wind_to = cfg.wind.bearing.Reciprocal();
  const double wind_east = cfg.wind.norm * wind_to.sin();
  const double wind_north = cfg.wind.norm * wind_to.cos();

  const unsigned n =
    unsigned(std::ceil(cfg.period_s * cfg.circles / cfg.dt_s)) + 2;

  double t = 0;
  for (unsigned i = 0; i < n; ++i) {
    const Angle heading =
      Angle::Degrees(360 * t / cfg.period_s);
    const double air_east = cfg.tas * heading.sin();
    const double air_north = cfg.tas * heading.cos();
    const SpeedVector gs(air_east + wind_east, air_north + wind_north);

    MoreData info;
    info.Reset();
    info.clock = TimeStamp{FloatDuration{t + 1}};
    info.time = info.clock;
    info.time_available.Update(info.clock);
    info.track = gs.bearing;
    info.track_available.Update(info.clock);
    info.ground_speed = gs.norm;
    info.ground_speed_available.Update(info.clock);
    if (cfg.report_tas) {
      info.true_airspeed = cfg.tas;
      info.airspeed_available.Update(info.clock);
      info.airspeed_real = true;
    }

    const auto result = circling_wind.NewSample(info, circling, cfg.vmin);
    if (result.IsValid())
      last = result;

    const double jitter =
      (i % 2) ? cfg.dt_jitter_s : -cfg.dt_jitter_s;
    t += cfg.dt_s + jitter;
  }

  return last;
}

static void
TestNotCircling() noexcept
{
  CirclingWind circling_wind;
  circling_wind.Reset();

  CirclingInfo circling;
  circling.Clear();

  MoreData info;
  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};
  info.time = info.clock;
  info.track = Angle::Zero();
  info.track_available.Update(info.clock);
  info.ground_speed = 10;
  info.ground_speed_available.Update(info.clock);

  const auto result = circling_wind.NewSample(info, circling);
  ok1(!result.IsValid());
}

static void
TestStillAir() noexcept
{
  /* Constant GPS track rate, no wind: a 20 s glider circle. */
  const auto result = FeedCircle({
    .tas = 25,
    .wind = SpeedVector::Zero(),
    .period_s = 20,
  });
  ok1(result.IsValid());
  ok1(result.wind.norm < 1.5);
}

static bool
WindMatches(const CirclingWind::Result &result, const SpeedVector &want,
            double speed_tol, Angle bearing_tol) noexcept
{
  if (!result.IsValid())
    return false;

  const double speed_err = std::fabs(result.wind.norm - want.norm);
  const double bearing_err =
    std::fabs((result.wind.bearing.AsBearing() - want.bearing)
                .AsDelta()
                .Degrees());
  if (speed_err > speed_tol || bearing_err > bearing_tol.Degrees()) {
    diag("got %.2f m/s %.1f deg, want %.2f m/s %.1f deg "
         "(speed_err %.2f, bearing_err %.1f)",
         result.wind.norm, result.wind.bearing.AsBearing().Degrees(),
         want.norm, want.bearing.Degrees(), speed_err, bearing_err);
    return false;
  }
  return true;
}

static void
TestGliderGpsWind() noexcept
{
  /* Typical glider: 25 m/s TAS, 5 m/s from west, GPS only. */
  const SpeedVector wind(Angle::Degrees(270), 5);
  const auto result = FeedCircle({
    .tas = 25,
    .wind = wind,
    .period_s = 20,
  });
  ok1(result.IsValid());
  ok1(WindMatches(result, wind, 1.5, Angle::Degrees(30)));
}

static void
TestGliderTasWind() noexcept
{
  const SpeedVector wind(Angle::Degrees(270), 5);
  const auto result = FeedCircle({
    .tas = 25,
    .wind = wind,
    .period_s = 20,
    .report_tas = true,
  });
  ok1(result.IsValid());
  ok1(WindMatches(result, wind, 1.5, Angle::Degrees(30)));
}

static void
TestParagliderGpsWind() noexcept
{
  /* Paraglider without TAS or gyro, 1 Hz GPS: 9 m/s TAS, 5 m/s from
     east, 25 s circles.  GPS track rate is not constant in wind when
     W/V is large — this is issue #2905. */
  const SpeedVector wind(Angle::Degrees(90), 5);
  const auto result = FeedCircle({
    .tas = 9,
    .wind = wind,
    .period_s = 25,
  });
  ok1(result.IsValid());
  ok1(WindMatches(result, wind, 2, Angle::Degrees(40)));
}

static void
TestParagliderReplayJitter() noexcept
{
  /* IGC replay timestamps come from the wall clock; 150 ms jitter
     exceeds the old 5 % (50 ms at 1 Hz) gate. */
  const SpeedVector wind(Angle::Degrees(90), 5);
  const auto result = FeedCircle({
    .tas = 9,
    .wind = wind,
    .period_s = 25,
    .dt_jitter_s = 0.15,
  });
  ok1(result.IsValid());
  ok1(WindMatches(result, wind, 2, Angle::Degrees(40)));
}

static void
TestParagliderThreeSecondFixes() noexcept
{
  /* Slow IGC logger / replay faster than 1×: ~3 s steps. */
  const SpeedVector wind(Angle::Degrees(90), 5);
  const auto result = FeedCircle({
    .tas = 9,
    .wind = wind,
    .period_s = 25,
    .dt_s = 3,
  });
  ok1(result.IsValid());
  ok1(WindMatches(result, wind, 2.5, Angle::Degrees(45)));
}

static void
TestParagliderPolarSpeedQuality() noexcept
{
  /* Matching polar min-sink keeps quality; a glider polar on a PG
     lowers it by one but still yields a wind. */
  const SpeedVector wind(Angle::Degrees(90), 5);
  CircleConfig cfg{
    .tas = 9,
    .wind = wind,
    .period_s = 25,
  };

  cfg.vmin = 9;
  const auto matched = FeedCircle(cfg);
  cfg.vmin = 25;
  const auto mismatched = FeedCircle(cfg);

  ok1(matched.IsValid());
  ok1(mismatched.IsValid());
  ok1(WindMatches(matched, wind, 2, Angle::Degrees(40)));
  ok1(matched.quality > mismatched.quality);
}

int
main()
{
  plan_tests(1 + 2 + 2 + 2 + 2 + 2 + 2 + 4);

  SetFakeLogFileQuiet(true);

  TestNotCircling();
  TestStillAir();
  TestGliderGpsWind();
  TestGliderTasWind();
  TestParagliderGpsWind();
  TestParagliderReplayJitter();
  TestParagliderThreeSecondFixes();
  TestParagliderPolarSpeedQuality();

  return exit_status();
}
