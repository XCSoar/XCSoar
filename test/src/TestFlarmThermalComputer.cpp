// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Computer/FlarmThermalComputer.hpp"
#include "FLARM/Calculations.hpp"
#include "FLARM/Computer.hpp"
#include "FLARM/Data.hpp"
#include "Geo/Math.hpp"
#include "Geo/SpeedVector.hpp"
#include "MapWindow/ThermalDisplay.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/ThermalProjection.hpp"
#include "FakeLogFile.hpp"
#include "TestUtil.hpp"

#include <algorithm>
#include <chrono>
#include <limits>

using namespace std::chrono;

static constexpr GeoPoint TEST_CENTRE = {
  Angle::Degrees(7), Angle::Degrees(45),
};
static constexpr double TEST_OWNSHIP_ALTITUDE = 900;

static void
TestThermalProjection()
{
  const SpeedVector wind{Angle::Degrees(90), 10};
  const auto drift_per_meter =
    CalculateThermalDriftPerMeter(wind, 2);
  ok1(equals(drift_per_meter.norm, 5));
  ok1(equals(drift_per_meter.bearing, 90));

  ok1(equals(ProjectThermalCore(TEST_CENTRE, 0, drift_per_meter),
              TEST_CENTRE));

  const auto expected_upwind =
    FindLatitudeLongitude(TEST_CENTRE, Angle::Degrees(270), 500);
  const auto expected_downwind =
    FindLatitudeLongitude(TEST_CENTRE, Angle::Degrees(90), 500);
  ok1(equals(ProjectThermalCore(TEST_CENTRE, 100, drift_per_meter),
              expected_upwind));
  ok1(equals(ProjectThermalCore(TEST_CENTRE, -100, drift_per_meter),
              expected_downwind));

  ok1(equals(ProjectThermalCore(TEST_CENTRE, 100,
                                SpeedVector::Zero(), 2),
              TEST_CENTRE));
  ok1(equals(ProjectThermalCore(TEST_CENTRE, 100, wind, 0),
              TEST_CENTRE));
  ok1(equals(ProjectThermalCore(TEST_CENTRE, 100, wind, -1),
              TEST_CENTRE));

  const double nan = std::numeric_limits<double>::quiet_NaN();
  ok1(equals(ProjectThermalCore(TEST_CENTRE, 100, wind, nan),
              TEST_CENTRE));
  ok1(equals(ProjectThermalCore(
                TEST_CENTRE, 100,
                SpeedVector{Angle::Degrees(90), nan}, 2),
              TEST_CENTRE));
  ok1(equals(ProjectThermalCore(
                TEST_CENTRE, 100,
                SpeedVector{Angle::Native(nan), 10}, 2),
              TEST_CENTRE));
  ok1(equals(ProjectThermalCore(
                TEST_CENTRE, 100,
                SpeedVector{Angle::Degrees(90), nan}),
              TEST_CENTRE));
  ok1(!ProjectThermalCore(GeoPoint::Invalid(), 100, drift_per_meter)
        .IsValid());

  const auto projected =
    ProjectThermalCore(TEST_CENTRE, 200, wind, 2);

  ThermalSource own_source{};
  own_source.location = TEST_CENTRE;
  own_source.ground_height = 1000;
  own_source.lift_rate = 2;
  ok1(equals(own_source.CalculateAdjustedLocation(1200, wind), projected));

  TrafficThermalSource traffic_source{};
  traffic_source.Clear();
  traffic_source.thermal.ground_height = 1000;
  traffic_source.reference_location = TEST_CENTRE;
  traffic_source.reference_altitude = 1000;
  traffic_source.drift_per_meter = drift_per_meter;
  ok1(equals(traffic_source.CalculateAdjustedLocation(1200), projected));

  ok1(equals(ThermalDisplay::GetLocation(own_source, 1200, wind),
              projected));
  ok1(equals(ThermalDisplay::GetLocation(traffic_source, 1200),
              projected));
  ok1(!ThermalDisplay::GetLocation(own_source, 999, wind).IsValid());
  ok1(!ThermalDisplay::GetLocation(traffic_source, 999).IsValid());
}

static GeoPoint
AdjustCoreToAltitude(GeoPoint centre, double altitude,
                     double reference_altitude,
                     double geometry_climb_rate,
                     const SpeedVector &wind)
{
  return ProjectThermalCore(centre, altitude - reference_altitude,
                            wind, geometry_climb_rate);
}

static void
TestTrafficThermalAllocation()
{
  TrafficThermalInfo info;
  info.Clear();

  auto &first = info.AllocateSource(1);
  first.first_seen = TimeStamp{seconds{1}};
  first.last_seen = first.first_seen;

  ok1(info.FindBySerial(1) == &first);
  ok1(&info.AllocateSource(1) == &first);

  for (unsigned i = 2; i <= TrafficThermalInfo::MAX_SOURCES; ++i) {
    auto &source = info.AllocateSource(i);
    source.first_seen = TimeStamp{seconds{i}};
    source.last_seen = source.first_seen;
  }

  auto &replacement = info.AllocateSource(TrafficThermalInfo::MAX_SOURCES + 1);
  ok1(info.sources.size() == TrafficThermalInfo::MAX_SOURCES);
  ok1(info.FindBySerial(1) == nullptr);
  ok1(replacement.cluster_serial == TrafficThermalInfo::MAX_SOURCES + 1);
  ok1(info.RemoveBySerial(replacement.cluster_serial));
  ok1(info.sources.size() == TrafficThermalInfo::MAX_SOURCES - 1);
}

static FlarmTraffic &
AppendTraffic(TrafficList &list, FlarmId id, TimeStamp time,
              GeoPoint centre, double climb_rate, bool circling=true,
              double radius=100)
{
  FlarmTraffic traffic{};
  traffic.Clear();
  traffic.id = id;
  traffic.id_type = FlarmTraffic::IdType::FLARM;
  traffic.source = FlarmTraffic::SourceType::FLARM;
  traffic.type = FlarmTraffic::AircraftType::GLIDER;
  traffic.valid.Update(time);
  traffic.location_available = true;
  traffic.altitude_available = true;
  traffic.speed_received = true;
  traffic.speed = 20;
  traffic.track_received = true;

  const double elapsed = time.ToDuration().count();
  const double turn = circling ? elapsed * 12 : 0;
  traffic.location = FindLatitudeLongitude(centre, Angle::Degrees(turn),
                                           radius);
  traffic.track = Angle::Degrees(circling ? turn + 90 : 90).AsBearing();
  traffic.altitude = 1000 + elapsed * climb_rate;
  traffic.relative_altitude =
    double(traffic.altitude) - TEST_OWNSHIP_ALTITUDE;
  traffic.climb_rate_avg30s = climb_rate;
  traffic.climb_rate_avg30s_time_span =
    FloatDuration{elapsed >= 31 ? 30 : std::max(0., elapsed - 1)};
  traffic.climb_rate_avg30s_available =
    traffic.climb_rate_avg30s_time_span >=
      FlarmCalculations::AVERAGE_TIME;
  traffic.climb_rate_avg30s_update =
    FlarmTraffic::Average30sUpdate::APPENDED;

  list.list.append(traffic);
  return list.list.back();
}

static FlarmTraffic &
AppendTrafficWithGeometry(TrafficList &list, FlarmId id, TimeStamp time,
                          GeoPoint centre, double reported_climb_rate,
                          double geometry_climb_rate,
                          const SpeedVector &geometry_wind,
                          double ownship_altitude,
                          bool circling=true, double radius=100,
                          double pressure_altitude_offset=0)
{
  auto &traffic = AppendTraffic(list, id, time, centre,
                                reported_climb_rate, circling, radius);
  const double elapsed = time.ToDuration().count();
  const double geometry_altitude = 1000 + elapsed * geometry_climb_rate;
  const double turn = circling ? elapsed * 12 : 0;
  const GeoPoint altitude_core = AdjustCoreToAltitude(
    centre, geometry_altitude, 1000, geometry_climb_rate, geometry_wind);

  traffic.location = FindLatitudeLongitude(altitude_core,
                                           Angle::Degrees(turn), radius);
  traffic.altitude = geometry_altitude + pressure_altitude_offset;
  traffic.relative_altitude = geometry_altitude - ownship_altitude;
  traffic.climb_rate_avg30s = reported_climb_rate;
  return traffic;
}

static void
RunSingleSequence(FlarmThermalComputer &computer, TrafficThermalInfo &info,
                  FlarmId id, double climb_rate, bool circling=true,
                  double radius=100,
                  FlarmTraffic::SourceType source=
                    FlarmTraffic::SourceType::FLARM)
{
  for (unsigned i = 1; i <= 31; ++i) {
    TrafficList list{};
    list.Clear();
    auto &traffic = AppendTraffic(list, id, TimeStamp{seconds{i}}, TEST_CENTRE,
                                  climb_rate, circling, radius);
    traffic.source = source;
    computer.Process(list, TimeStamp{seconds{i}}, TEST_OWNSHIP_ALTITUDE,
                     SpeedVector::Zero(), nullptr, info);
    if (i == 30)
      ok1(info.sources.empty());
  }
}

static void
TestQualificationAndLifecycle()
{
  TrafficThermalInfo info;
  info.Clear();
  FlarmThermalComputer computer;
  computer.Reset(info);

  RunSingleSequence(computer, info, FlarmId::FromValue(1), 1.5);
  ok1(info.sources.size() == 1);
  ok1(info.sources.front().aircraft_count == 1);
  ok1(info.sources.front().active_aircraft_count == 1);
  ok1(info.sources.front().active);
  ok1(equals(info.sources.front().thermal.lift_rate, 1.5));
  ok1(equals(info.sources.front().thermal.ground_height, 0));
  ok1(equals(info.sources.front().min_observed_altitude, 1002));
  ok1(equals(info.sources.front().max_observed_altitude, 1047));
  const double min_altitude = info.sources.front().min_observed_altitude;
  const double max_altitude = info.sources.front().max_observed_altitude;

  TrafficList duplicate{};
  duplicate.Clear();
  auto &duplicate_traffic =
    AppendTraffic(duplicate, FlarmId::FromValue(1),
                  TimeStamp{seconds{31}}, TEST_CENTRE, 1.5);
  duplicate_traffic.climb_rate_avg30s_update =
    FlarmTraffic::Average30sUpdate::REPLACED;
  computer.Process(duplicate, TimeStamp{seconds{31}},
                   TEST_OWNSHIP_ALTITUDE, SpeedVector::Zero(), nullptr,
                   info);
  ok1(info.sources.size() == 1);
  ok1(info.sources.front().aircraft_count == 1);

  TrafficList empty{};
  empty.Clear();
  computer.Process(empty, TimeStamp{seconds{42}}, TEST_OWNSHIP_ALTITUDE,
                   SpeedVector::Zero(), nullptr, info);
  ok1(!info.sources.front().active);
  ok1(info.sources.front().active_aircraft_count == 0);
  ok1(equals(info.sources.front().thermal.lift_rate, 1.5));
  ok1(equals(info.sources.front().min_observed_altitude, min_altitude));
  ok1(equals(info.sources.front().max_observed_altitude, max_altitude));

  computer.Process(empty, TimeStamp{seconds{20}}, TEST_OWNSHIP_ALTITUDE,
                   SpeedVector::Zero(), nullptr, info);
  ok1(info.sources.empty());
}

static void
TestRejectedTracks()
{
  TrafficThermalInfo weak_info;
  weak_info.Clear();
  FlarmThermalComputer weak;
  weak.Reset(weak_info);
  RunSingleSequence(weak, weak_info, FlarmId::FromValue(2), 0.4);
  ok1(weak_info.sources.empty());

  TrafficThermalInfo straight_info;
  straight_info.Clear();
  FlarmThermalComputer straight;
  straight.Reset(straight_info);
  RunSingleSequence(straight, straight_info, FlarmId::FromValue(3), 1.5,
                    false);
  ok1(straight_info.sources.empty());

  TrafficThermalInfo spread_info;
  spread_info.Clear();
  FlarmThermalComputer spread;
  spread.Reset(spread_info);
  RunSingleSequence(spread, spread_info, FlarmId::FromValue(4), 1.5, true,
                    600);
  ok1(spread_info.sources.empty());

  TrafficThermalInfo online_info;
  online_info.Clear();
  FlarmThermalComputer online;
  online.Reset(online_info);
  RunSingleSequence(online, online_info, FlarmId::FromValue(5), 1.5, true,
                    100, FlarmTraffic::SourceType::OGN);
  ok1(online_info.sources.empty());
}

static void
TestGrouping()
{
  TrafficThermalInfo info;
  info.Clear();
  FlarmThermalComputer computer;
  computer.Reset(info);

  const GeoPoint second_centre =
    FindLatitudeLongitude(TEST_CENTRE, Angle::Degrees(90), 200);
  for (unsigned i = 1; i <= 31; ++i) {
    TrafficList list{};
    list.Clear();
    AppendTraffic(list, FlarmId::FromValue(10), TimeStamp{seconds{i}},
                  TEST_CENTRE, 1);
    auto &second = AppendTraffic(list, FlarmId::FromValue(11),
                                 TimeStamp{seconds{i}}, second_centre, 2);
    second.altitude = double(second.altitude) + 300;
    second.relative_altitude = double(second.relative_altitude) + 300;
    computer.Process(list, TimeStamp{seconds{i}}, TEST_OWNSHIP_ALTITUDE,
                     SpeedVector::Zero(), nullptr, info);
  }

  ok1(info.sources.size() == 1);
  ok1(info.sources.front().aircraft_count == 2);
  ok1(info.sources.front().active_aircraft_count == 2);
  ok1(equals(info.sources.front().thermal.lift_rate, 1.5));
  ok1(info.sources.front().min_observed_altitude < 1100);
  ok1(info.sources.front().max_observed_altitude > 1350);

  for (unsigned i = 32; i <= 42; ++i) {
    TrafficList only_second{};
    only_second.Clear();
    auto &second = AppendTraffic(only_second, FlarmId::FromValue(11),
                                 TimeStamp{seconds{i}}, second_centre, 2);
    second.altitude = double(second.altitude) + 300;
    second.relative_altitude = double(second.relative_altitude) + 300;
    computer.Process(only_second, TimeStamp{seconds{i}},
                     TEST_OWNSHIP_ALTITUDE, SpeedVector::Zero(), nullptr,
                     info);
  }
  ok1(info.sources.front().aircraft_count == 2);
  ok1(info.sources.front().active_aircraft_count == 1);
  ok1(equals(info.sources.front().thermal.lift_rate, 2));
}

static void
TestSeparateThermals()
{
  TrafficThermalInfo info;
  info.Clear();
  FlarmThermalComputer computer;
  computer.Reset(info);

  const GeoPoint distant_centre =
    FindLatitudeLongitude(TEST_CENTRE, Angle::Degrees(90), 1200);
  for (unsigned i = 1; i <= 31; ++i) {
    TrafficList list{};
    list.Clear();
    AppendTraffic(list, FlarmId::FromValue(20), TimeStamp{seconds{i}},
                  TEST_CENTRE, 1.5);
    AppendTraffic(list, FlarmId::FromValue(21), TimeStamp{seconds{i}},
                  distant_centre, 1.5);
    computer.Process(list, TimeStamp{seconds{i}}, TEST_OWNSHIP_ALTITUDE,
                     SpeedVector::Zero(), nullptr, info);
  }

  ok1(info.sources.size() == 2);
}

static void
TestInvalidAltitudePreservesRange()
{
  TrafficThermalInfo info;
  info.Clear();
  FlarmThermalComputer computer;
  computer.Reset(info);

  RunSingleSequence(computer, info, FlarmId::FromValue(22), 1.5);
  const double minimum = info.sources.front().min_observed_altitude;
  const double maximum = info.sources.front().max_observed_altitude;

  TrafficList invalid{};
  invalid.Clear();
  auto &traffic = AppendTraffic(invalid, FlarmId::FromValue(22),
                                TimeStamp{seconds{32}}, TEST_CENTRE, 1.5);
  traffic.altitude_available = false;
  computer.Process(invalid, TimeStamp{seconds{32}}, TEST_OWNSHIP_ALTITUDE,
                   SpeedVector::Zero(), nullptr, info);

  ok1(equals(info.sources.front().min_observed_altitude, minimum));
  ok1(equals(info.sources.front().max_observed_altitude, maximum));
  ok1(!info.sources.front().active);
}

static void
TestMergePreservesAltitudeRange()
{
  TrafficThermalInfo info;
  info.Clear();
  FlarmThermalComputer computer;
  computer.Reset(info);

  for (unsigned i = 1; i <= 31; ++i) {
    TrafficList list{};
    list.Clear();
    AppendTraffic(list, FlarmId::FromValue(23), TimeStamp{seconds{i}},
                  TEST_CENTRE, 1.5);
    const GeoPoint distant = FindLatitudeLongitude(
      TEST_CENTRE, Angle::Degrees(90), 900);
    auto &second = AppendTraffic(list, FlarmId::FromValue(24),
                                 TimeStamp{seconds{i}}, distant, 1.5);
    second.altitude = double(second.altitude) + 300;
    second.relative_altitude = double(second.relative_altitude) + 300;
    computer.Process(list, TimeStamp{seconds{i}}, TEST_OWNSHIP_ALTITUDE,
                     SpeedVector::Zero(), nullptr, info);
  }
  ok1(info.sources.size() == 2);

  for (unsigned i = 32; i <= 95; ++i) {
    const double distance = i <= 62
      ? 900 - (i - 31) * (600. / 31.)
      : 300;
    const GeoPoint converging = FindLatitudeLongitude(
      TEST_CENTRE, Angle::Degrees(90), distance);

    TrafficList list{};
    list.Clear();
    AppendTraffic(list, FlarmId::FromValue(23), TimeStamp{seconds{i}},
                  TEST_CENTRE, 1.5);
    auto &second = AppendTraffic(list, FlarmId::FromValue(24),
                                 TimeStamp{seconds{i}}, converging, 1.5);
    second.altitude = double(second.altitude) + 300;
    second.relative_altitude = double(second.relative_altitude) + 300;
    computer.Process(list, TimeStamp{seconds{i}}, TEST_OWNSHIP_ALTITUDE,
                     SpeedVector::Zero(), nullptr, info);
  }

  ok1(info.sources.size() == 1);
  ok1(info.sources.front().aircraft_count == 2);
  ok1(info.sources.front().min_observed_altitude < 1100 &&
      info.sources.front().max_observed_altitude > 1400);
}

static void
TestStraightDepartureFreezesSource()
{
  TrafficThermalInfo info;
  info.Clear();
  FlarmThermalComputer computer;
  computer.Reset(info);

  RunSingleSequence(computer, info, FlarmId::FromValue(30), 1.5);
  ok1(info.sources.front().active);
  const GeoPoint qualified_location =
    info.sources.front().CalculateAdjustedLocation(TEST_OWNSHIP_ALTITUDE);

  for (unsigned i = 32; i <= 33; ++i) {
    TrafficList list{};
    list.Clear();
    AppendTraffic(list, FlarmId::FromValue(30), TimeStamp{seconds{i}},
                  TEST_CENTRE, 1.5, false);
    computer.Process(list, TimeStamp{seconds{i}}, TEST_OWNSHIP_ALTITUDE,
                     SpeedVector::Zero(), nullptr, info);
  }

  ok1(!info.sources.front().active);
  ok1(info.sources.front().active_aircraft_count == 0);
  ok1(qualified_location.DistanceS(
        info.sources.front().CalculateAdjustedLocation(
          TEST_OWNSHIP_ALTITUDE)) < 50);

  TrafficList straight{};
  straight.Clear();
  AppendTraffic(straight, FlarmId::FromValue(30), TimeStamp{seconds{34}},
                TEST_CENTRE, 1.5, false);
  computer.Process(straight, TimeStamp{seconds{34}},
                   TEST_OWNSHIP_ALTITUDE, SpeedVector::Zero(), nullptr,
                   info);
  ok1(info.sources.size() == 1);
}

static void
TestStableGeometryAndAltitudeDatum()
{
  TrafficThermalInfo info;
  info.Clear();
  FlarmThermalComputer computer;
  computer.Reset(info);

  const SpeedVector qualification_wind{Angle::Degrees(70), 10};
  for (unsigned i = 1; i <= 31; ++i) {
    TrafficList list{};
    list.Clear();
    AppendTrafficWithGeometry(list, FlarmId::FromValue(31),
                              TimeStamp{seconds{i}}, TEST_CENTRE,
                              1.5, 1.5, qualification_wind,
                              TEST_OWNSHIP_ALTITUDE, true, 100, 400);
    computer.Process(list, TimeStamp{seconds{i}},
                     TEST_OWNSHIP_ALTITUDE, qualification_wind, nullptr,
                     info);
  }

  ok1(info.sources.size() == 1);
  ok1(info.sources.front().max_observed_altitude < 1200);
  ok1(equals(info.sources.front().geometry_lift_rate, 1.5));
  ok1(equals(info.sources.front().geometry_wind.norm, 10));

  const GeoPoint display_at_900 =
    info.sources.front().CalculateAdjustedLocation(900);
  const GeoPoint expected_at_900 = AdjustCoreToAltitude(
    TEST_CENTRE, 900, 1000, 1.5, qualification_wind);
  ok1(display_at_900.DistanceS(expected_at_900) < 50);

  const SpeedVector changed_wind{Angle::Degrees(250), 20};
  for (unsigned i = 32; i <= 35; ++i) {
    const double ownship_altitude = 900 + (i - 31) * 50;
    TrafficList list{};
    list.Clear();
    AppendTrafficWithGeometry(list, FlarmId::FromValue(31),
                              TimeStamp{seconds{i}}, TEST_CENTRE,
                              2.5, 1.5, qualification_wind,
                              ownship_altitude, true, 100, 400);
    computer.Process(list, TimeStamp{seconds{i}}, ownship_altitude,
                     changed_wind, nullptr, info);
  }

  ok1(equals(info.sources.front().thermal.lift_rate, 2.5));
  ok1(equals(info.sources.front().geometry_lift_rate, 1.5));
  ok1(equals(info.sources.front().geometry_wind.norm, 10));
  ok1(display_at_900.DistanceS(
        info.sources.front().CalculateAdjustedLocation(900)) < 50);

  const GeoPoint expected_at_1200 = AdjustCoreToAltitude(
    TEST_CENTRE, 1200, 1000, 1.5, qualification_wind);
  ok1(info.sources.front().CalculateAdjustedLocation(1200)
        .DistanceS(expected_at_1200) < 50);

  const GeoPoint active_location =
    info.sources.front().CalculateAdjustedLocation(1100);
  TrafficList empty{};
  empty.Clear();
  computer.Process(empty, TimeStamp{seconds{46}}, 1100, changed_wind,
                   nullptr, info);

  ok1(!info.sources.front().active);
  ok1(info.sources.front().thermal.lift_rate > 1.5 &&
      info.sources.front().thermal.lift_rate < 2.5);
  ok1(active_location.DistanceS(
        info.sources.front().CalculateAdjustedLocation(1100)) < 0.1);
  ok1(equals(info.sources.front().geometry_lift_rate, 1.5));
}

static void
TestThermalVisibility()
{
  ok1(ThermalDisplay::IsVisible(4000));
  ok1(ThermalDisplay::IsVisible(3999));
  ok1(!ThermalDisplay::IsVisible(4001));
  ok1(ThermalDisplay::IsTrafficVisible(true, 4000));
  ok1(!ThermalDisplay::IsTrafficVisible(true, 4001));
  ok1(!ThermalDisplay::IsTrafficVisible(false, 1000));
}

static void
TestPublishedSamplingPolicy()
{
  TrafficThermalInfo info;
  info.Clear();
  FlarmThermalComputer computer;
  computer.Reset(info);

  for (unsigned i = 1; i <= 31; ++i) {
    TrafficList list{};
    list.Clear();
    auto &traffic = AppendTraffic(list, FlarmId::FromValue(32),
                                  TimeStamp{seconds{i}}, TEST_CENTRE, 1.5);
    if (i == 31) {
      traffic.climb_rate_avg30s_available = true;
      traffic.climb_rate_avg30s_time_span = seconds{29};
    }

    computer.Process(list, TimeStamp{seconds{i}}, TEST_OWNSHIP_ALTITUDE,
                     SpeedVector::Zero(), nullptr, info);
  }

  ok1(info.sources.empty());

  TrafficList complete{};
  complete.Clear();
  AppendTraffic(complete, FlarmId::FromValue(32), TimeStamp{seconds{32}},
                TEST_CENTRE, 1.5);
  computer.Process(complete, TimeStamp{seconds{32}},
                   TEST_OWNSHIP_ALTITUDE, SpeedVector::Zero(), nullptr,
                   info);
  ok1(info.sources.size() == 1);
  const auto location = info.sources.front().thermal.location;

  TrafficList ignored{};
  ignored.Clear();
  auto &ignored_traffic =
    AppendTraffic(ignored, FlarmId::FromValue(32),
                  TimeStamp{FloatDuration{32.1}}, TEST_CENTRE, 1.5, true,
                  600);
  ignored_traffic.climb_rate_avg30s_update =
    FlarmTraffic::Average30sUpdate::IGNORED;
  computer.Process(ignored, TimeStamp{FloatDuration{32.1}},
                   TEST_OWNSHIP_ALTITUDE, SpeedVector::Zero(), nullptr,
                   info);

  ok1(info.sources.front().thermal.location.DistanceS(location) < 0.1);
  ok1(info.sources.front().aircraft_count == 1);
}

static FlarmData
MakeAverageTraffic(TimeStamp valid_time, double altitude)
{
  FlarmData data;
  data.Clear();

  auto *traffic = data.traffic.AllocateTraffic();
  traffic->Clear();
  traffic->id = FlarmId::FromValue(33);
  traffic->id_type = FlarmTraffic::IdType::RANDOM;
  traffic->valid.Update(valid_time);
  traffic->relative_north = 0;
  traffic->relative_east = 0;
  traffic->absolute_altitude = true;
  traffic->altitude_available = true;
  traffic->altitude = altitude;
  traffic->track_received = true;
  traffic->turn_rate_received = true;
  traffic->speed_received = true;
  traffic->climb_rate_received = true;
  return data;
}

static void
TestPublishedSamplingActionPersistence()
{
  FlarmComputer computer;
  FlarmData empty;
  empty.Clear();

  NMEAInfo basic;
  basic.Reset();
  basic.clock = TimeStamp{seconds{100}};
  basic.ProvideTime(TimeStamp{seconds{1}});

  auto first = MakeAverageTraffic(TimeStamp{seconds{10}}, 1000);
  computer.Process(first, empty, basic);
  const auto &first_traffic = first.traffic.list.front();
  ok1(first_traffic.climb_rate_avg30s_update ==
      FlarmTraffic::Average30sUpdate::APPENDED);
  ok1(!first_traffic.climb_rate_avg30s_reset);

  auto repeated = first;
  computer.Process(repeated, first, basic);
  const auto &repeated_traffic = repeated.traffic.list.front();
  ok1(repeated_traffic.climb_rate_avg30s_update ==
      FlarmTraffic::Average30sUpdate::APPENDED);
  ok1(!repeated_traffic.climb_rate_avg30s_reset);
  ok1(repeated_traffic.climb_rate_avg30s_time_span ==
      first_traffic.climb_rate_avg30s_time_span);

  basic.clock = TimeStamp{seconds{106}};
  basic.ProvideTime(TimeStamp{seconds{7}});
  auto after_gap = repeated;
  after_gap.traffic.list.front().valid.Update(TimeStamp{seconds{20}});
  after_gap.traffic.list.front().altitude = 1010;
  computer.Process(after_gap, repeated, basic);
  const auto &gap_traffic = after_gap.traffic.list.front();
  ok1(gap_traffic.climb_rate_avg30s_update ==
      FlarmTraffic::Average30sUpdate::APPENDED);
  ok1(gap_traffic.climb_rate_avg30s_reset);

  auto repeated_gap = after_gap;
  computer.Process(repeated_gap, after_gap, basic);
  const auto &repeated_gap_traffic = repeated_gap.traffic.list.front();
  ok1(repeated_gap_traffic.climb_rate_avg30s_update ==
      FlarmTraffic::Average30sUpdate::APPENDED);
  ok1(repeated_gap_traffic.climb_rate_avg30s_reset);
}

int
main()
{
  plan_tests(108);
  SetFakeLogFileQuiet(true);

  TestThermalProjection();
  TestTrafficThermalAllocation();
  TestQualificationAndLifecycle();
  TestRejectedTracks();
  TestGrouping();
  TestSeparateThermals();
  TestInvalidAltitudePreservesRange();
  TestMergePreservesAltitudeRange();
  TestStraightDepartureFreezesSource();
  TestStableGeometryAndAltitudeDatum();
  TestThermalVisibility();
  TestPublishedSamplingPolicy();
  TestPublishedSamplingActionPersistence();

  return exit_status();
}
