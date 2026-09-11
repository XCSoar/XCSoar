// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGC/FlightTimes.hpp"
#include "IGC/ApplyFixToNMEA.hpp"
#include "Computer/FlyingComputer.hpp"
#include "NMEA/Derived.hpp"
#include "NMEA/Info.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/Operation.hpp"
#include "TestUtil.hpp"
#include "system/Path.hpp"
#include "util/PrintException.hxx"

#include <cstdlib>

class CancelledOperationEnvironment final : public NullOperationEnvironment {
public:
  bool IsCancelled() const noexcept override {
    return true;
  }
};

static void
CheckFixAdapter()
{
  IGCFix fix;
  fix.Clear();
  fix.location = GeoPoint(Angle::Degrees(7), Angle::Degrees(50));
  fix.gps_valid = true;
  fix.gps_altitude = 1234;
  fix.pressure_altitude = 1200;
  fix.enl = 321;
  fix.rpm = 600;
  fix.trt = 42;
  fix.gsp = -1;
  fix.ias = 72;
  fix.tas = 90;
  fix.siu = 8;

  NMEAInfo basic;
  basic.Reset();
  const TimeStamp time{FloatDuration{90001}};
  ApplyIGCFixToNMEA(fix, time, 12.5, basic);

  ok1(basic.alive && basic.time_available && basic.location_available);
  ok1(basic.time == time);
  ok1(basic.date_time_utc.hour == 1);
  ok1(basic.location == fix.location);
  ok1(basic.gps_altitude == 1234);
  ok1(basic.pressure_altitude == 1200);
  ok1(basic.ground_speed == 12.5);
  ok1(basic.indicated_airspeed == 20);
  ok1(basic.true_airspeed == 25);
  ok1(basic.engine_noise_level == 321);
  ok1(basic.engine.revolutions_per_second == 10);
  ok1(basic.track == Angle::Degrees(42));
  ok1(basic.gps.satellites_used == 8);

  fix.gps_altitude = fix.pressure_altitude = 0;
  fix.ClearExtensions();
  ApplyIGCFixToNMEA(fix, TimeStamp{FloatDuration{90002}}, std::nullopt,
                    basic);
  ok1(!basic.gps_altitude_available);
  ok1(!basic.pressure_altitude_available);
  ok1(!basic.baro_altitude_available);
  ok1(!basic.ground_speed_available);
  ok1(!basic.airspeed_available);
  ok1(!basic.engine_noise_level_available);
  ok1(!basic.engine.revolutions_per_second_available);
  ok1(!basic.track_available);
  ok1(!basic.gps.satellites_used_available);
}

static void
UpdateFlying(FlyingComputer &computer, FlyingState &flying,
             NMEAInfo &basic, const DerivedInfo &calculated,
             unsigned second, double ground_speed, double altitude,
             double takeoff_speed=10)
{
  const TimeStamp time{FloatDuration{static_cast<double>(second)}};
  basic.clock = time;
  basic.time = time;
  basic.time_available.Update(time);
  basic.location = GeoPoint(Angle::Degrees(7), Angle::Degrees(50));
  basic.location_available.Update(time);
  basic.ground_speed = ground_speed;
  basic.ground_speed_available.Update(time);
  basic.gps_altitude = altitude;
  basic.gps_altitude_available.Update(time);
  computer.Compute(takeoff_speed, basic, calculated, flying);
}

static void
PrepareFlying(FlyingComputer &computer, FlyingState &flying,
              NMEAInfo &basic, DerivedInfo &calculated)
{
  computer.Reset();
  flying.Reset();
  basic.Reset();
  calculated.Reset();

  for (unsigned i = 0; i <= 12; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 0, 100);
}

static void
CheckLowSpeedClimb()
{
  FlyingComputer computer;
  FlyingState flying;
  NMEAInfo basic;
  DerivedInfo calculated;
  PrepareFlying(computer, flying, basic, calculated);

  for (unsigned i = 13; i <= 45; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3,
                 100 + (i - 12));

  ok1(flying.flying);
  ok1(flying.takeoff_time == TimeStamp{FloatDuration{13}});
}

static void
CheckHighSpeedLaunchTimestamp()
{
  FlyingComputer computer;
  FlyingState flying;
  NMEAInfo basic;
  DerivedInfo calculated;
  PrepareFlying(computer, flying, basic, calculated);

  for (unsigned i = 13; i <= 23; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 10, 100);

  ok1(flying.flying);
  ok1(flying.takeoff_time == TimeStamp{FloatDuration{13}});
}

static void
CheckResumedLowSpeedClimb()
{
  FlyingComputer computer;
  FlyingState flying;
  NMEAInfo basic;
  DerivedInfo calculated;
  PrepareFlying(computer, flying, basic, calculated);

  for (unsigned i = 13; i <= 17; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3, 100 + i - 12);
  for (unsigned i = 18; i <= 24; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3, 105);
  for (unsigned i = 25; i <= 35; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3,
                 105 + 2 * (i - 24));

  ok1(flying.flying);
  ok1(flying.takeoff_time == TimeStamp{FloatDuration{25}});
}

static void
CheckLowSpeedCandidateSpeedReset()
{
  FlyingComputer computer;
  FlyingState flying;
  NMEAInfo basic;
  DerivedInfo calculated;
  PrepareFlying(computer, flying, basic, calculated);

  for (unsigned i = 13; i <= 20; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3, 100 + i - 12);
  UpdateFlying(computer, flying, basic, calculated, 21, 2.9, 109);
  for (unsigned i = 22; i <= 32; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3,
                 109 + 2 * (i - 21));

  ok1(flying.flying);
  ok1(flying.takeoff_time == TimeStamp{FloatDuration{22}});
}

static void
CheckLowSpeedGroundMovement()
{
  FlyingComputer computer;
  FlyingState flying;
  NMEAInfo basic;
  DerivedInfo calculated;
  PrepareFlying(computer, flying, basic, calculated);

  for (unsigned i = 13; i <= 43; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3,
                 100 + (i - 12) * 0.2);

  ok1(!flying.flying);
}

static void
CheckBelowLowSpeedThreshold()
{
  FlyingComputer computer;
  FlyingState flying;
  NMEAInfo basic;
  DerivedInfo calculated;
  PrepareFlying(computer, flying, basic, calculated);

  for (unsigned i = 13; i <= 55; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 2.9,
                 100 + (i - 12));

  ok1(!flying.flying);
}

static void
CheckInterruptedClimb()
{
  FlyingComputer computer;
  FlyingState flying;
  NMEAInfo basic;
  DerivedInfo calculated;
  PrepareFlying(computer, flying, basic, calculated);

  for (unsigned i = 13; i <= 22; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3,
                 100 + (i - 12));
  for (unsigned i = 23; i <= 27; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3,
                 110 - (i - 22));
  for (unsigned i = 28; i <= 58; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3, 105);

  ok1(!flying.flying);
}

static void
CheckLandingAfterHighSpeedClimb()
{
  FlyingComputer computer;
  FlyingState flying;
  NMEAInfo basic;
  DerivedInfo calculated;
  PrepareFlying(computer, flying, basic, calculated);

  for (unsigned i = 13; i <= 45; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 12,
                 100 + (i - 12));
  for (unsigned i = 46; i <= 80; ++i)
    UpdateFlying(computer, flying, basic, calculated, i, 3.9, 133);

  ok1(!flying.flying);
  ok1(flying.landing_time == TimeStamp{FloatDuration{46}});
}

static void
CheckCalculated()
{
  const auto result = DetectIGCFlightTimes(Path("test/data/flight_times.igc"));
  ok1(result.has_valid_fixes);
  ok1(result.takeoff_detected);
  ok1(result.landing_detected);
  ok1(result.takeoff == BrokenTime(12, 0, 5));
  ok1(result.landing == BrokenTime(12, 0, 45));
  ok1(result.duration == std::chrono::seconds(40));
}

static void
CheckFallback()
{
  const auto result =
    DetectIGCFlightTimes(Path("test/data/flight_times_stationary.igc"));
  ok1(!result.takeoff_detected);
  ok1(!result.landing_detected);
  ok1(result.takeoff == BrokenTime(12, 0, 0));
  ok1(result.landing == BrokenTime(12, 0, 9));
  ok1(result.duration == std::chrono::seconds(9));
}

static void
CheckInvalidFixes()
{
  const auto result =
    DetectIGCFlightTimes(Path("test/data/flight_times_invalid.igc"));
  ok1(!result.has_valid_fixes);
}

static void
CheckCancellation()
{
  CancelledOperationEnvironment operation;
  bool cancelled = false;
  try {
    DetectIGCFlightTimes(Path("test/data/flight_times.igc"), &operation);
  } catch (const OperationCancelled &) {
    cancelled = true;
  }

  ok1(cancelled);
}

static void
CheckFileError()
{
  bool failed = false;
  try {
    DetectIGCFlightTimes(Path("test/data/does-not-exist.igc"));
  } catch (...) {
    failed = true;
  }

  ok1(failed);
}

static void
CheckMidnight()
{
  const auto result =
    DetectIGCFlightTimes(Path("test/data/flight_times_midnight.igc"));
  ok1(result.takeoff == BrokenTime(23, 59, 35));
  ok1(result.landing == BrokenTime(0, 0, 15));
  ok1(result.duration == std::chrono::seconds(40));
}

static void
CheckForwardTimeJump()
{
  const auto result =
    DetectIGCFlightTimes(Path("test/data/flight_times_forward_jump.igc"));
  ok1(result.has_valid_fixes);
  ok1(result.takeoff == BrokenTime(1, 0, 0));
  ok1(result.landing == BrokenTime(1, 0, 1));
  ok1(result.duration == std::chrono::seconds(1));
}

static void
CheckMultipleFlights()
{
  const auto result =
    DetectIGCFlightTimes(Path("test/data/flight_times_multiple.igc"));
  ok1(result.takeoff == BrokenTime(12, 0, 5));
  ok1(result.landing == BrokenTime(12, 2, 5));
  ok1(result.duration == std::chrono::seconds(120));
}

static void
CheckBundledFlight(Path path)
{
  const auto result = DetectIGCFlightTimes(path);
  ok1(result.has_valid_fixes);
  ok1(result.duration > std::chrono::minutes(30));
}

int
main()
try {
  plan_tests(63);
  CheckFixAdapter();
  CheckCalculated();
  CheckFallback();
  CheckInvalidFixes();
  CheckCancellation();
  CheckFileError();
  CheckMidnight();
  CheckForwardTimeJump();
  CheckMultipleFlights();
  CheckBundledFlight(Path("test/data/01lz1hq1.igc"));
  CheckBundledFlight(Path("test/data/0asljd01.igc"));
  CheckLowSpeedClimb();
  CheckHighSpeedLaunchTimestamp();
  CheckResumedLowSpeedClimb();
  CheckLowSpeedCandidateSpeedReset();
  CheckLowSpeedGroundMovement();
  CheckBelowLowSpeedThreshold();
  CheckInterruptedClimb();
  CheckLandingAfterHighSpeedClimb();
  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
