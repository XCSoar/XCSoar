// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Engine/Observation/Store.hpp"
#include "TestUtil.hpp"

#include <chrono>

using namespace std::chrono;

static GeoPoint
P(double lon, double lat) noexcept
{
  return GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
}

static ObservedThermal
Row(ObservationOrigin origin, GeoPoint location,
    steady_clock::time_point received) noexcept
{
  ObservedThermal row;
  row.origin = origin;
  row.location = location;
  row.received = received;
  return row;
}

static unsigned
Count(const ObservationStore &store, ObservationOrigin origin) noexcept
{
  unsigned n = 0;
  for (const auto &row : store.Thermals())
    if (row.origin == origin)
      ++n;
  return n;
}

int
main()
{
  plan_tests(24);

  const GeoPoint here = P(10, 50);
  const GeoPoint near = P(10, 50.005);
  const GeoPoint far = P(10, 50.02);
  const auto t0 = steady_clock::time_point{};
  const auto t1 = t0 + seconds{1};

  ok1(here.DistanceS(near) < THERMAL_DUPLICATE_RANGE_M);
  ok1(here.DistanceS(far) > THERMAL_DUPLICATE_RANGE_M);

  ObservationStore store;
  ObservedThermal own = Row(ObservationOrigin::OWN, here, t0);
  store.ReplaceThermals(ObservationOrigin::OWN, {&own, 1});
  ObservedThermal cloud = Row(ObservationOrigin::CLOUD, far, t0);
  store.ReplaceThermals(ObservationOrigin::CLOUD, {&cloud, 1});
  ok1(store.Thermals().size() == 2);
  ok1(Count(store, ObservationOrigin::OWN) == 1);
  ok1(Count(store, ObservationOrigin::CLOUD) == 1);

  store.ReplaceThermals(ObservationOrigin::OWN, {});
  ok1(store.Thermals().size() == 1);
  ok1(store.Thermals()[0].origin == ObservationOrigin::CLOUD);

  store.ReplaceThermals(ObservationOrigin::CLOUD, {});
  store.ReplaceThermals(ObservationOrigin::OWN, {&own, 1});
  for (unsigned i = 0; i < OBSERVATION_CAP + 1; ++i) {
    ObservedThermal row = Row(ObservationOrigin::CLOUD,
                              P(11, 40 + 0.01 * i), t0 + seconds{i});
    store.AppendThermal(row);
  }
  ok1(Count(store, ObservationOrigin::CLOUD) == OBSERVATION_CAP);
  ok1(Count(store, ObservationOrigin::OWN) == 1);
  ok1(store.Thermals()[0].origin == ObservationOrigin::OWN);

  bool dropped_first = true;
  for (const auto &row : store.Thermals())
    if (row.origin == ObservationOrigin::CLOUD &&
        row.location.DistanceS(P(11, 40)) < 1)
      dropped_first = false;
  ok1(dropped_first);

  ThermalRows shown;
  const ObservedThermal close[] = {
    Row(ObservationOrigin::OWN, here, t0),
    Row(ObservationOrigin::CLOUD, near, t1),
    Row(ObservationOrigin::TIM, far, t0),
  };
  SelectThermals(close, shown, THERMAL_DUPLICATE_RANGE_M);
  ok1(shown.size() == 2);
  ok1(shown[0].origin == ObservationOrigin::CLOUD);
  ok1(shown[1].origin == ObservationOrigin::TIM);

  const ObservedThermal newer_first[] = {
    Row(ObservationOrigin::CLOUD, near, t1),
    Row(ObservationOrigin::OWN, here, t0),
  };
  SelectThermals(newer_first, shown, THERMAL_DUPLICATE_RANGE_M);
  ok1(shown.size() == 1);
  ok1(shown[0].origin == ObservationOrigin::CLOUD);

  /* A and C do not overlap. B overlaps both and is no newer than C.
     Dropping A while rejecting B would lose a marker. */
  const GeoPoint other = P(10, 50.012);
  const GeoPoint mid = P(10, 50.006);
  ok1(here.DistanceS(other) > THERMAL_DUPLICATE_RANGE_M);
  ok1(here.DistanceS(mid) < THERMAL_DUPLICATE_RANGE_M);
  ok1(other.DistanceS(mid) < THERMAL_DUPLICATE_RANGE_M);

  const ObservedThermal chain[] = {
    Row(ObservationOrigin::OWN, here, t0),
    Row(ObservationOrigin::TIM, other, t1),
    Row(ObservationOrigin::CLOUD, mid, t1),
  };
  SelectThermals(chain, shown, THERMAL_DUPLICATE_RANGE_M);
  ok1(shown.size() == 2);
  ok1(shown[0].origin == ObservationOrigin::OWN);
  ok1(shown[1].origin == ObservationOrigin::TIM);

  ok1(!OriginShown(ObservationOrigin::CLOUD, false, true));
  ok1(OriginShown(ObservationOrigin::SKYLINES, false, false));

  return exit_status();
}
