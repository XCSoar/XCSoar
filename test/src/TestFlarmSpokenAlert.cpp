// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/SpokenAlert.hpp"
#include "FLARM/Status.hpp"
#include "TestUtil.hpp"

#include <cstring>

using Vertical = SpokenTrafficAlert::Vertical;

/**
 * A PFLAU aircraft alarm; a bearing below -180 means non-directional.
 */
static FlarmStatus
MakeAlarm(FlarmTraffic::AlarmType level, int bearing, int vertical,
          unsigned distance) noexcept
{
  FlarmStatus status;
  status.Clear();
  status.available.Update(TimeStamp{FloatDuration{1}});
  status.alarm_level = level;
  status.has_extended = true;
  status.has_bearing = bearing >= -180;
  status.relative_bearing = status.has_bearing ? bearing : 0;
  status.alarm_type = 2;
  status.has_vertical = true;
  status.relative_vertical = vertical;
  status.relative_distance = distance;
  return status;
}

static bool
ClipsAre(const SpokenTrafficAlert &alert, const char *a,
         const char *b = nullptr, const char *c = nullptr) noexcept
{
  const auto clips = alert.GetClips();
  const char *expected[] = {a, b, c};
  for (unsigned i = 0; i < 3; ++i) {
    const bool present = i < clips.size();
    if (present != (expected[i] != nullptr) ||
        (present && strcmp(clips[i], expected[i]) != 0))
      return false;
  }

  return true;
}

static void
TestClock()
{
  /* bearing -> clock position, each hour 30 degrees wide */
  static constexpr struct {
    int bearing;
    unsigned clock;
  } cases[] = {
    {0, 12}, {14, 12}, {15, 1}, {30, 1}, {44, 1}, {45, 2},
    {90, 3}, {179, 6}, {180, 6}, {-180, 6}, {-90, 9},
    {-30, 11}, {-15, 12}, {-16, 11},
  };

  for (const auto &i : cases) {
    const auto alert = SpokenTrafficAlert::FromStatus(
      MakeAlarm(FlarmTraffic::AlarmType::LOW, i.bearing, 0, 1000));
    ok(alert && alert->clock == i.clock, "bearing %d -> %u o'clock",
       i.bearing, i.clock);
  }
}

static void
TestVertical()
{
  /* above or below beyond 15% of the distance */
  auto alert = SpokenTrafficAlert::FromStatus(
    MakeAlarm(FlarmTraffic::AlarmType::IMPORTANT, 90, 200, 1000));
  ok1(alert && alert->vertical == Vertical::ABOVE);

  alert = SpokenTrafficAlert::FromStatus(
    MakeAlarm(FlarmTraffic::AlarmType::IMPORTANT, 90, -200, 1000));
  ok1(alert && alert->vertical == Vertical::BELOW);

  alert = SpokenTrafficAlert::FromStatus(
    MakeAlarm(FlarmTraffic::AlarmType::IMPORTANT, 90, 100, 1000));
  ok1(alert && alert->vertical == Vertical::LEVEL);

  /* without a distance, the vertical position is not known */
  alert = SpokenTrafficAlert::FromStatus(
    MakeAlarm(FlarmTraffic::AlarmType::IMPORTANT, 90, 100, 0));
  ok1(alert && alert->vertical == Vertical::UNKNOWN);
}

static void
TestClips()
{
  /* the example of the "Test spoken alert" button */
  ok1(ClipsAre(SpokenTrafficAlert::Example(), "IDR_WAV_TRAFFIC_URGENT",
               "IDR_WAV_OCLOCK_12", "IDR_WAV_ABOVE"));

  auto alert = SpokenTrafficAlert::FromStatus(
    MakeAlarm(FlarmTraffic::AlarmType::URGENT, 90, 300, 600));
  ok1(alert && ClipsAre(*alert, "IDR_WAV_TRAFFIC_URGENT", "IDR_WAV_OCLOCK_3",
                        "IDR_WAV_ABOVE"));

  alert = SpokenTrafficAlert::FromStatus(
    MakeAlarm(FlarmTraffic::AlarmType::LOW, -60, 0, 2000));
  ok1(alert && ClipsAre(*alert, "IDR_WAV_TRAFFIC_LOW", "IDR_WAV_OCLOCK_10",
                        "IDR_WAV_LEVEL"));

  /* non-directional (PCAS): no clock position */
  alert = SpokenTrafficAlert::FromStatus(
    MakeAlarm(FlarmTraffic::AlarmType::IMPORTANT, -999, -150, 500));
  ok1(alert && alert->clock == 0);
  ok1(alert && ClipsAre(*alert, "IDR_WAV_TRAFFIC_IMPORTANT",
                        "IDR_WAV_BELOW"));

  /* neither direction nor distance: the urgency only */
  alert = SpokenTrafficAlert::FromStatus(
    MakeAlarm(FlarmTraffic::AlarmType::URGENT, -999, -150, 0));
  ok1(alert && ClipsAre(*alert, "IDR_WAV_TRAFFIC_URGENT"));

  /* neither direction nor vertical distance (PFLAU fields empty): the
     urgency only, not "level" */
  auto status = MakeAlarm(FlarmTraffic::AlarmType::URGENT, -999, 0, 500);
  status.has_vertical = false;
  status.has_extended = false;
  alert = SpokenTrafficAlert::FromStatus(status);
  ok1(alert && alert->vertical == Vertical::UNKNOWN);
  ok1(alert && ClipsAre(*alert, "IDR_WAV_TRAFFIC_URGENT"));
}

static void
TestNoAlert()
{
  ok1(!SpokenTrafficAlert::FromStatus(
        MakeAlarm(FlarmTraffic::AlarmType::NONE, 0, 0, 1000)));
  ok1(!SpokenTrafficAlert::FromStatus(
        MakeAlarm(FlarmTraffic::AlarmType::INFO_ALERT, 0, 0, 1000)));

  /* an obstacle (3) or an alert zone (0x41) is not traffic */
  auto status = MakeAlarm(FlarmTraffic::AlarmType::URGENT, 0, 0, 1000);
  status.alarm_type = 3;
  ok1(!SpokenTrafficAlert::FromStatus(status));
  status.alarm_type = 0x41;
  ok1(!SpokenTrafficAlert::FromStatus(status));

  /* no FLARM */
  status = MakeAlarm(FlarmTraffic::AlarmType::URGENT, 0, 0, 1000);
  status.available.Clear();
  ok1(!SpokenTrafficAlert::FromStatus(status));
}

int
main()
{
  plan_tests(14 + 4 + 8 + 5);

  TestClock();
  TestVertical();
  TestClips();
  TestNoAlert();

  return exit_status();
}
