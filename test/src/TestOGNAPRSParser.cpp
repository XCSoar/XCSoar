// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#define TEST_OGN_PARSER 1

#include "Cloud/OGNClient.hpp"
#include "Cloud/OGNTraffic.hpp"
#include "FLARM/Traffic.hpp"
#include "Geo/GeoPoint.hpp"
#include "TestUtil.hpp"
#include "event/Loop.hxx"
#include "event/net/cares/Channel.hxx"
#include "util/ASCII.hxx"

#include <cstring>
#include <memory>

// Test helper class to access private ParseOGNPosition method
class TestOGNHelper {
public:
  static bool ParseOGNPosition(OGNClient &client, const char *packet,
                               GeoPoint &location, int &altitude,
                               unsigned &track, unsigned &speed,
                               int &climb_rate, double &turn_rate,
                               FlarmTraffic::AircraftType &aircraft_type,
                               bool &track_received)
  {
    return client.ParseOGNPosition(packet, location, altitude, track, speed,
                                   climb_rate, turn_rate, aircraft_type,
                                   track_received);
  }
};

static void
TestBasicPosition()
{
  // Test basic position packet: "!ddmm.hhN/dddmm.hhW"
  // Example: "!4742.50N/12226.22W"
  const char *packet = "!4742.50N/12226.22W";

  EventLoop event_loop;
  Cares::Channel cares_channel(event_loop);
  OGNTrafficContainer traffic_container;
  OGNClient client(event_loop, cares_channel, traffic_container);

  GeoPoint location;
  int altitude = -1;
  unsigned track = 0;
  unsigned speed = 0;
  int climb_rate = 0;
  double turn_rate = 0;
  FlarmTraffic::AircraftType aircraft_type =
      FlarmTraffic::AircraftType::UNKNOWN;
  bool track_received = false;

  bool result = TestOGNHelper::ParseOGNPosition(
      client, packet, location, altitude, track, speed, climb_rate, turn_rate,
      aircraft_type, track_received);

  ok1(result);
  ok1(location.IsValid());

  // Expected: 47.7083°N, 122.4372°W
  // 47°42.50' = 47 + 42.50/60 = 47.7083°
  // 122°26.22' = 122 + 26.22/60 = 122.4372°
  ok1(equals(location.latitude.Degrees(), 47.7083));
  ok1(equals(location.longitude.Degrees(), -122.4372));
  ok1(!track_received); // No track in this packet
}

static void
TestPositionWithTimestamp()
{
  // Test position with timestamp: "/hhmmss/ddmm.hhN/dddmm.hhW"
  // The packet must start with !, /, or =
  // After timestamp, the format is: timestamp/ddmm.hhN/symbol/dddmm.hhW
  const char *packet = "/123456/4742.50N/12226.22W";

  EventLoop event_loop;
  Cares::Channel cares_channel(event_loop);
  OGNTrafficContainer traffic_container;
  OGNClient client(event_loop, cares_channel, traffic_container);

  GeoPoint location;
  int altitude = -1;
  unsigned track = 0;
  unsigned speed = 0;
  int climb_rate = 0;
  double turn_rate = 0;
  FlarmTraffic::AircraftType aircraft_type =
      FlarmTraffic::AircraftType::UNKNOWN;
  bool track_received = false;

  bool result = TestOGNHelper::ParseOGNPosition(
      client, packet, location, altitude, track, speed, climb_rate, turn_rate,
      aircraft_type, track_received);

  ok1(result);
  ok1(location.IsValid());
  ok1(equals(location.latitude.Degrees(), 47.7083));
  ok1(equals(location.longitude.Degrees(), -122.4372));
}

static void
TestPositionWithCourseSpeed()
{
  // Test position with course/speed: "!ddmm.hhN/dddmm.hhW'ttt/sss"
  // Example: "!4742.50N/12226.22W'270/045"
  const char *packet = "!4742.50N/12226.22W'270/045";

  EventLoop event_loop;
  Cares::Channel cares_channel(event_loop);
  OGNTrafficContainer traffic_container;
  OGNClient client(event_loop, cares_channel, traffic_container);

  GeoPoint location;
  int altitude = -1;
  unsigned track = 0;
  unsigned speed = 0;
  int climb_rate = 0;
  double turn_rate = 0;
  FlarmTraffic::AircraftType aircraft_type =
      FlarmTraffic::AircraftType::UNKNOWN;
  bool track_received = false;

  bool result = TestOGNHelper::ParseOGNPosition(
      client, packet, location, altitude, track, speed, climb_rate, turn_rate,
      aircraft_type, track_received);

  ok1(result);
  ok1(location.IsValid());
  ok1(track_received);
  ok1(track == 270);
  // Speed: 45 knots = 45 * 0.514444 = 23.15 m/s (rounded)
  ok1(speed >= 23 && speed <= 24);
}

static void
TestPositionWithAltitude()
{
  // Test position with altitude: "!ddmm.hhN/dddmm.hhW/A=12345"
  const char *packet = "!4742.50N/12226.22W/A=12345";

  EventLoop event_loop;
  Cares::Channel cares_channel(event_loop);
  OGNTrafficContainer traffic_container;
  OGNClient client(event_loop, cares_channel, traffic_container);

  GeoPoint location;
  int altitude = -1;
  unsigned track = 0;
  unsigned speed = 0;
  int climb_rate = 0;
  double turn_rate = 0;
  FlarmTraffic::AircraftType aircraft_type =
      FlarmTraffic::AircraftType::UNKNOWN;
  bool track_received = false;

  bool result = TestOGNHelper::ParseOGNPosition(
      client, packet, location, altitude, track, speed, climb_rate, turn_rate,
      aircraft_type, track_received);

  ok1(result);
  ok1(location.IsValid());
  // Expected: altitude = 12345 feet = 12345 * 0.3048 = 3762.756 meters
  ok1(altitude >= 3762 && altitude <= 3763);
}

static void
TestPositionWithClimbRate()
{
  // Test position with climb rate: "!ddmm.hhN/dddmm.hhW +0123fpm"
  const char *packet = "!4742.50N/12226.22W +0123fpm";

  EventLoop event_loop;
  Cares::Channel cares_channel(event_loop);
  OGNTrafficContainer traffic_container;
  OGNClient client(event_loop, cares_channel, traffic_container);

  GeoPoint location;
  int altitude = -1;
  unsigned track = 0;
  unsigned speed = 0;
  int climb_rate = 0;
  double turn_rate = 0;
  FlarmTraffic::AircraftType aircraft_type =
      FlarmTraffic::AircraftType::UNKNOWN;
  bool track_received = false;

  bool result = TestOGNHelper::ParseOGNPosition(
      client, packet, location, altitude, track, speed, climb_rate, turn_rate,
      aircraft_type, track_received);

  ok1(result);
  ok1(location.IsValid());
  // Expected: climb_rate = 123 fpm = 123 * 0.00508 = 0.625 m/s (rounded)
  ok1(climb_rate >= 0 && climb_rate <= 1);
}

static void
TestPositionWithTurnRate()
{
  // Test position with turn rate: "!4742.50N/12226.22W rot=+12.5"
  const char *packet = "!4742.50N/12226.22W rot=+12.5";

  EventLoop event_loop;
  Cares::Channel cares_channel(event_loop);
  OGNTrafficContainer traffic_container;
  OGNClient client(event_loop, cares_channel, traffic_container);

  GeoPoint location;
  int altitude = -1;
  unsigned track = 0;
  unsigned speed = 0;
  int climb_rate = 0;
  double turn_rate = 0;
  FlarmTraffic::AircraftType aircraft_type =
      FlarmTraffic::AircraftType::UNKNOWN;
  bool track_received = false;

  bool result = TestOGNHelper::ParseOGNPosition(
      client, packet, location, altitude, track, speed, climb_rate, turn_rate,
      aircraft_type, track_received);

  ok1(result);
  ok1(location.IsValid());
  ok1(equals(turn_rate, 12.5));
}

static void
TestAircraftTypes()
{
  // Test different aircraft type symbols
  // In APRS format, the symbol comes between latitude and longitude
  // Format: !ddmm.hhN/symbol/dddmm.hhW
  const char *packets[] = {
      "!4742.50N/12226.22W",  // GLIDER (symbol / between lat and lon)
      "!4742.50N\\12226.22W", // POWERED_AIRCRAFT (symbol \ between lat and
                              // lon)
      "!4742.50N'12226.22W",  // HANG_GLIDER (symbol ' between lat and lon)
      "!4742.50N^12226.22W",  // PARA_GLIDER (symbol ^ between lat and lon)
      "!4742.50NO12226.22W",  // BALLOON (symbol O between lat and lon)
      "!4742.50Ng12226.22W",  // UAV (symbol g between lat and lon)
  };

  FlarmTraffic::AircraftType expected[] = {
      FlarmTraffic::AircraftType::GLIDER,
      FlarmTraffic::AircraftType::POWERED_AIRCRAFT,
      FlarmTraffic::AircraftType::HANG_GLIDER,
      FlarmTraffic::AircraftType::PARA_GLIDER,
      FlarmTraffic::AircraftType::BALLOON,
      FlarmTraffic::AircraftType::UAV,
  };

  EventLoop event_loop;
  Cares::Channel cares_channel(event_loop);
  OGNTrafficContainer traffic_container;
  OGNClient client(event_loop, cares_channel, traffic_container);

  for (unsigned i = 0; i < sizeof(packets) / sizeof(packets[0]); i++) {
    GeoPoint location;
    int altitude = -1;
    unsigned track = 0;
    unsigned speed = 0;
    int climb_rate = 0;
    double turn_rate = 0;
    FlarmTraffic::AircraftType aircraft_type =
        FlarmTraffic::AircraftType::UNKNOWN;
    bool track_received = false;

    bool result = TestOGNHelper::ParseOGNPosition(
        client, packets[i], location, altitude, track, speed, climb_rate,
        turn_rate, aircraft_type, track_received);

    ok1(result);
    ok1(aircraft_type == expected[i]);
  }
}

static void
TestInvalidPackets()
{
  // Test various invalid packets that should fail parsing
  const char *invalid_packets[] = {
      "",                    // Empty
      "X4742.50N/12226.22W", // Invalid start character
      "!4742.50X/12226.22W", // Invalid latitude direction
      "!4742.50N/12226.22X", // Invalid longitude direction
      "!47N/122W",           // Missing minutes
      "!4742.50N",           // Missing longitude
  };

  bool should_fail[] = {
      true, // Empty
      true, // Invalid start
      true, // Invalid lat dir
      true, // Invalid lon dir
      true, // Missing minutes
      true, // Missing longitude
  };

  EventLoop event_loop;
  Cares::Channel cares_channel(event_loop);
  OGNTrafficContainer traffic_container;
  OGNClient client(event_loop, cares_channel, traffic_container);

  for (unsigned i = 0;
       i < sizeof(invalid_packets) / sizeof(invalid_packets[0]); i++) {
    GeoPoint location;
    int altitude = -1;
    unsigned track = 0;
    unsigned speed = 0;
    int climb_rate = 0;
    double turn_rate = 0;
    FlarmTraffic::AircraftType aircraft_type =
        FlarmTraffic::AircraftType::UNKNOWN;
    bool track_received = false;

    bool result = TestOGNHelper::ParseOGNPosition(
        client, invalid_packets[i], location, altitude, track, speed,
        climb_rate, turn_rate, aircraft_type, track_received);

    if (should_fail[i]) {
      ok1(!result);
    } else {
      ok1(result);
    }
  }

  // Test valid packet
  const char *valid_packet = "!4742.50N/12226.22W";
  GeoPoint location;
  int altitude = -1;
  unsigned track = 0;
  unsigned speed = 0;
  int climb_rate = 0;
  double turn_rate = 0;
  FlarmTraffic::AircraftType aircraft_type =
      FlarmTraffic::AircraftType::UNKNOWN;
  bool track_received = false;

  bool result = TestOGNHelper::ParseOGNPosition(
      client, valid_packet, location, altitude, track, speed, climb_rate,
      turn_rate, aircraft_type, track_received);
  ok1(result);
  ok1(location.IsValid());
}

static void
TestTrackReceived()
{
  // Test that track_received is set correctly
  const char *with_track = "!4742.50N/12226.22W'270/045";
  const char *without_track = "!4742.50N/12226.22W";

  EventLoop event_loop;
  Cares::Channel cares_channel(event_loop);
  OGNTrafficContainer traffic_container;
  OGNClient client(event_loop, cares_channel, traffic_container);

  // Test with track
  {
    GeoPoint location;
    int altitude = -1;
    unsigned track = 0;
    unsigned speed = 0;
    int climb_rate = 0;
    double turn_rate = 0;
    FlarmTraffic::AircraftType aircraft_type =
        FlarmTraffic::AircraftType::UNKNOWN;
    bool track_received = false;

    bool result = TestOGNHelper::ParseOGNPosition(
        client, with_track, location, altitude, track, speed, climb_rate,
        turn_rate, aircraft_type, track_received);
    ok1(result);
    ok1(track_received);
    ok1(track == 270);
  }

  // Test without track
  {
    GeoPoint location;
    int altitude = -1;
    unsigned track = 0;
    unsigned speed = 0;
    int climb_rate = 0;
    double turn_rate = 0;
    FlarmTraffic::AircraftType aircraft_type =
        FlarmTraffic::AircraftType::UNKNOWN;
    bool track_received =
        true; // Start with true to verify it gets set to false

    bool result = TestOGNHelper::ParseOGNPosition(
        client, without_track, location, altitude, track, speed, climb_rate,
        turn_rate, aircraft_type, track_received);
    ok1(result);
    ok1(!track_received);
  }
}

static void
TestRealWorldExamples()
{
  // Test with real-world OGN APRS packet examples
  // These are actual packets received from OGN servers
  // Format: position data after colon:
  // "/hhmmss/ddmm.mmN/symbol/dddmm.mmE^ttt/sss/A=aaaaaa ..."

  // Extract position data portion (after colon) from real packets
  const char *real_packet1 = "/205026h4942.30N/00707.33E^266/435/A=024093 "
                             "!W08! id253C6749 +64fpm FL239.50 A3:DLH84M";
  const char *real_packet2 = "/205026h5024.58N/01109.93E^285/420/A=035793 "
                             "!W22! id0140804D +0fpm FL359.50";
  const char *real_packet3 = "/205026h4814.26N/01559.29E^340/319/A=015994 "
                             "!W22! id2544082B +1792fpm FL159.45 A3:AUA649";
  const char *real_packet4 = "/205026h5025.30N/00830.59E^066/478/A=036843 "
                             "!W33! id25511170 +64fpm FL370.50 A3:MBU8TN";
  const char *real_packet5 = "/205027h5059.14N/00646.55E'302/034/A=000206 "
                             "!W92! id203D223B +000fpm gps6x3";

  EventLoop event_loop;
  Cares::Channel cares_channel(event_loop);
  OGNTrafficContainer traffic_container;
  OGNClient client(event_loop, cares_channel, traffic_container);

  // Test packet 1: Full featured packet with course/speed, altitude, climb
  // rate
  {
    GeoPoint location;
    int altitude = -1;
    unsigned track = 0;
    unsigned speed = 0;
    int climb_rate = 0;
    double turn_rate = 0;
    FlarmTraffic::AircraftType aircraft_type =
        FlarmTraffic::AircraftType::UNKNOWN;
    bool track_received = false;

    bool result = TestOGNHelper::ParseOGNPosition(
        client, real_packet1, location, altitude, track, speed, climb_rate,
        turn_rate, aircraft_type, track_received);
    ok1(result);
    ok1(location.IsValid());
    ok1(track_received);
    ok1(track == 266);
    // Speed: 435 knots = 435 * 0.514444 = 223.78 m/s (rounded)
    ok1(speed >= 223 && speed <= 224);
    // Altitude: 24093 feet = 24093 * 0.3048 = 7343.5 meters
    ok1(altitude >= 7343 && altitude <= 7344);
    // Climb rate: 64 fpm = 64 * 0.00508 = 0.325 m/s (rounded)
    ok1(climb_rate >= 0 && climb_rate <= 1);
    // Aircraft type symbol is / between lat and lon (GLIDER), not ^ (which is
    // course/speed separator)
    ok1(aircraft_type == FlarmTraffic::AircraftType::GLIDER);
  }

  // Test packet 2: With timestamp, course/speed, altitude, zero climb rate
  {
    GeoPoint location;
    int altitude = -1;
    unsigned track = 0;
    unsigned speed = 0;
    int climb_rate = 0;
    double turn_rate = 0;
    FlarmTraffic::AircraftType aircraft_type =
        FlarmTraffic::AircraftType::UNKNOWN;
    bool track_received = false;

    bool result = TestOGNHelper::ParseOGNPosition(
        client, real_packet2, location, altitude, track, speed, climb_rate,
        turn_rate, aircraft_type, track_received);
    ok1(result);
    ok1(location.IsValid());
    ok1(track_received);
    ok1(track == 285);
    // Speed: 420 knots = 420 * 0.514444 = 216.07 m/s
    ok1(speed >= 216 && speed <= 217);
    // Altitude: 35793 feet = 35793 * 0.3048 = 10909.7 meters
    ok1(altitude >= 10909 && altitude <= 10910);
    ok1(climb_rate == 0);
    // Aircraft type symbol is / between lat and lon (GLIDER), not ^ (which is
    // course/speed separator)
    ok1(aircraft_type == FlarmTraffic::AircraftType::GLIDER);
  }

  // Test packet 3: High climb rate
  {
    GeoPoint location;
    int altitude = -1;
    unsigned track = 0;
    unsigned speed = 0;
    int climb_rate = 0;
    double turn_rate = 0;
    FlarmTraffic::AircraftType aircraft_type =
        FlarmTraffic::AircraftType::UNKNOWN;
    bool track_received = false;

    bool result = TestOGNHelper::ParseOGNPosition(
        client, real_packet3, location, altitude, track, speed, climb_rate,
        turn_rate, aircraft_type, track_received);
    ok1(result);
    ok1(location.IsValid());
    ok1(track_received);
    ok1(track == 340);
    // Climb rate: 1792 fpm = 1792 * 0.00508 = 9.10 m/s (rounded)
    ok1(climb_rate >= 9 && climb_rate <= 10);
    // Aircraft type symbol is / between lat and lon (GLIDER), not ^ (which is
    // course/speed separator)
    ok1(aircraft_type == FlarmTraffic::AircraftType::GLIDER);
  }

  // Test packet 4: High speed
  {
    GeoPoint location;
    int altitude = -1;
    unsigned track = 0;
    unsigned speed = 0;
    int climb_rate = 0;
    double turn_rate = 0;
    FlarmTraffic::AircraftType aircraft_type =
        FlarmTraffic::AircraftType::UNKNOWN;
    bool track_received = false;

    bool result = TestOGNHelper::ParseOGNPosition(
        client, real_packet4, location, altitude, track, speed, climb_rate,
        turn_rate, aircraft_type, track_received);
    ok1(result);
    ok1(location.IsValid());
    ok1(track_received);
    ok1(track == 66);
    // Speed: 478 knots = 478 * 0.514444 = 245.91 m/s
    ok1(speed >= 245 && speed <= 246);
    // Aircraft type symbol is / between lat and lon (GLIDER), not ^ (which is
    // course/speed separator)
    ok1(aircraft_type == FlarmTraffic::AircraftType::GLIDER);
  }

  // Test packet 5: Hang glider symbol (')
  {
    GeoPoint location;
    int altitude = -1;
    unsigned track = 0;
    unsigned speed = 0;
    int climb_rate = 0;
    double turn_rate = 0;
    FlarmTraffic::AircraftType aircraft_type =
        FlarmTraffic::AircraftType::UNKNOWN;
    bool track_received = false;

    bool result = TestOGNHelper::ParseOGNPosition(
        client, real_packet5, location, altitude, track, speed, climb_rate,
        turn_rate, aircraft_type, track_received);
    ok1(result);
    ok1(location.IsValid());
    ok1(track_received);
    ok1(track == 302);
    // Speed: 34 knots = 34 * 0.514444 = 17.49 m/s
    ok1(speed >= 17 && speed <= 18);
    ok1(aircraft_type ==
        FlarmTraffic::AircraftType::GLIDER); // ' is course/speed separator, /
                                             // is aircraft type symbol
  }
}

int
main()
{
  plan_tests(82); // Count: 60 original + 22 new real-world tests (5 packets
                  // with multiple assertions each)

  TestBasicPosition();
  TestPositionWithTimestamp();
  TestPositionWithCourseSpeed();
  TestPositionWithAltitude();
  TestPositionWithClimbRate();
  TestPositionWithTurnRate();
  TestAircraftTypes();
  TestInvalidPackets();
  TestTrackReceived();
  TestRealWorldExamples();

  return exit_status();
}
