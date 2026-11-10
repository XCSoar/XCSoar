// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Tracking/LiveTrack24/Protocol.hpp"
#include "Geo/GeoPoint.hpp"
#include "TestUtil.hpp"

using namespace LiveTrack24;

/* The test vector from the LiveTracking API v2 manual. */
static void
TestToken()
{
  ok1(MakeToken("thisIsMyPass", 605419896, "This-Is-An-App-Secret") ==
      "xjFqBpRZR2trZltB5C2pRg");

  /* the password is case-insensitive */
  ok1(MakeToken("THISISMYPASS", 605419896, "This-Is-An-App-Secret") ==
      "xjFqBpRZR2trZltB5C2pRg");

  /* another session gives another token */
  ok1(MakeToken("thisIsMyPass", 0, "This-Is-An-App-Secret") !=
      "xjFqBpRZR2trZltB5C2pRg");
}

static void
TestParseReply()
{
  auto reply = ParseReply("0;OK\n12345\n");
  ok1(reply && reply->IsOK() && reply->message == "OK" &&
      reply->data == "12345");

  reply = ParseReply("0;OK\r\n12345\r\n");
  ok1(reply && reply->message == "OK" && reply->data == "12345");

  reply = ParseReply("0;OK");
  ok1(reply && reply->IsOK() && reply->data.empty());

  reply = ParseReply("22;Wrong password");
  ok1(reply && reply->code == 22 && !reply->IsOK() &&
      reply->message == "Wrong password");

  ok1(!ParseReply(""));
  ok1(!ParseReply("<html>Just a moment...</html>"));
  ok1(!ParseReply(";OK"));
  ok1(!ParseReply("x;OK"));
}

static constexpr AppInfo app{"KEY", "7.47", "xcsoar"};

static void
TestPaths()
{
  /* keep '%' out of ok1() expressions: tap prints them with printf */
  const std::string login = LoginPath(app, "tok", "a b:c");
  const char *const login_expected =
    "/api/t/lt/getUserID/KEY/7.47/xcsoar/0/0/tok/a%20b%3Ac";
  ok1(login == login_expected);

  Session session;
  session.id = 123;
  session.user = 45;
  session.token = "T-_";

  const GeoPoint munich{Angle::Degrees(11.5), Angle::Degrees(48.25)};
  const Fix fix{
    std::chrono::system_clock::from_time_t(1700000000),
    munich, 500, 90, 270,
  };

  const TrackInfo info{VehicleType::GLIDER, "ASW 20", "XCSoar"};
  const std::string first = TrackPath(app, session, 1, fix, &info);
  const char *const first_expected =
    "/api/t/lt/track/KEY/7.47/xcsoar/123/45/T-_/0/99/1/1700000000/"
    "48.250000/11.500000/500/90/270/8:ASW%2020:XCSoar";
  ok1(first == first_expected);

  ok1(TrackPath(app, session, 2, fix, nullptr) ==
      "/api/t/lt/track/KEY/7.47/xcsoar/123/45/T-_/0/99/2/1700000000/"
      "48.250000/11.500000/500/90/270");

  const GeoPoint santiago{Angle::Degrees(-70.25), Angle::Degrees(-33.5)};
  const Fix south{
    std::chrono::system_clock::from_time_t(1700000000),
    santiago, -3, 0, 0,
  };
  ok1(TrackPath(app, session, 3, south, nullptr) ==
      "/api/t/lt/track/KEY/7.47/xcsoar/123/45/T-_/0/99/3/1700000000/"
      "-33.500000/-70.250000/-3/0/0");

  ok1(TrackEndPath(app, session, 7) ==
      "/api/t/lt/trackEnd/KEY/7.47/xcsoar/123/45/T-_/0/99/7/0/1");
}

int
main()
{
  plan_tests(3 + 8 + 5);

  TestToken();
  TestParseReply();
  TestPaths();

  return exit_status();
}
