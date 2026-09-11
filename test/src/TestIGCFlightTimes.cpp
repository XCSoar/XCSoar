// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "IGC/FlightTimes.hpp"
#include "IGC/ApplyFixToNMEA.hpp"
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
  plan_tests(50);
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
  return exit_status();
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
